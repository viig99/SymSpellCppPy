#include "library.h"

#include <charconv>
#include <deque>
#include <fstream>
#include <optional>
#include <utility>

#include "cereal/archives/binary.hpp"
#include "cereal/archives/portable_binary.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/unordered_map.hpp"
#include "cereal/types/vector.hpp"

namespace symspellcpppy
{

    int SymSpell::MaxDictionaryEditDistance() const
    {
        return maxDictionaryEditDistance;
    }

    int SymSpell::PrefixLength() const
    {
        return prefixLength;
    }

    int SymSpell::MaxLength() const
    {
        return maxDictionaryWordLength;
    }

    int SymSpell::CountThreshold() const
    {
        return countThreshold;
    }

    int SymSpell::WordCount()
    {
        return words.size();
    }

    int SymSpell::EntryCount()
    {
        return deletes.size();
    }

    SymSpell::SymSpell(int _maxDictionaryEditDistance, int _prefixLength, int _countThreshold, int _initialCapacity,
                       unsigned char _compactLevel) : initialCapacity(_initialCapacity),
                                                      maxDictionaryEditDistance(_maxDictionaryEditDistance),
                                                      prefixLength(_prefixLength),
                                                      countThreshold(_countThreshold),
                                                      compactLevel(_compactLevel)
    {
        if (_initialCapacity < 0)
            throw std::invalid_argument("initial_capacity is too small.");
        if (_maxDictionaryEditDistance < 0)
            throw std::invalid_argument("max_dictionary_edit_distance cannot be negative");
        if (_prefixLength < 1 || _prefixLength <= _maxDictionaryEditDistance)
            throw std::invalid_argument(
                "prefix_length cannot be less than 1 or smaller than max_dictionary_edit_distance");
        if (_countThreshold < 0)
            throw std::invalid_argument("count_threshold cannot be negative");
        if (_compactLevel > 16)
            throw std::invalid_argument("compact_level cannot be greater than 16");

        compactMask = (UINT_MAX >> (3 + _compactLevel)) << 2;
        maxDictionaryWordLength = 0;
        words = std::unordered_map<xstring, int64_t>(initialCapacity);
    }

    SymSpell::SymSpell(const SymSpell &other)
        : SymSpell(other.maxDictionaryEditDistance, other.prefixLength, other.countThreshold,
                   other.initialCapacity, other.compactLevel)
    {
        words = other.words;
        belowThresholdWords = other.belowThresholdWords;
        maxDictionaryWordLength = other.maxDictionaryWordLength;
        bigrams = other.bigrams;
        bigramCountMin = other.bigramCountMin;
        deletes.reserve(other.deletes.size());
        for (const auto &[hash, entries] : other.deletes)
        {
            auto &bucket = deletes[hash];
            bucket.reserve(entries.size());
            for (const auto *word : entries)
                bucket.push_back(&*words.find(word->first));
        }
    }

    SymSpell &SymSpell::operator=(const SymSpell &other)
    {
        return *this = SymSpell(other);
    }

    bool SymSpell::CreateDictionaryEntry(const xstring &key, int64_t count,
                                         const std::shared_ptr<SuggestionStage> &staging)
    {

        if (count <= 0)
        {
            if (countThreshold > 0)
                return false; // no point doing anything if count is zero, as it can't change anything
            count = 0;
        }
        int64_t countPrevious;
        auto belowThresholdWordsFinded = belowThresholdWords.find(key);
        auto wordsFinded = words.find(key);
        if (countThreshold > 1 && belowThresholdWordsFinded != belowThresholdWords.end())
        {
            countPrevious = belowThresholdWordsFinded->second;
            count = (INT64_MAX - countPrevious > count) ? countPrevious + count : INT64_MAX;
            if (count >= countThreshold)
            {
                belowThresholdWords.erase(key);
            }
            else
            {
                belowThresholdWords[key] = count;
                return false;
            }
        }
        else if (wordsFinded != words.end())
        {
            countPrevious = wordsFinded->second;
            count = (INT64_MAX - countPrevious > count) ? countPrevious + count : INT64_MAX;
            wordsFinded->second = count;
            return false;
        }
        else if (count < CountThreshold())
        {
            belowThresholdWords[key] = count;
            return false;
        }

        const auto *word = &*words.emplace(key, count).first;

        maxDictionaryWordLength = std::max(maxDictionaryWordLength, static_cast<int>(key.size()));

        // create deletes
        auto edits = EditsPrefix(key);
        if (staging != nullptr)
        {
            const auto *stagedWord = staging->TrackWord(key);
            for (const auto &edit : *edits)
                staging->Add(GetstringHash(edit), stagedWord);
        }
        else
        {

            for (const auto &edit : *edits)
                deletes[GetstringHash(edit)].push_back(word);
        }

        return true;
    }

    bool SymSpell::DeleteDictionaryEntry(const xstring &key)
    {
        const auto found = words.find(key);
        if (found == words.end())
            return false;

        const auto edits = EditsPrefix(key);
        for (const auto &edit : *edits)
        {
            const auto bucket = deletes.find(GetstringHash(edit));
            if (bucket == deletes.end())
                continue;
            std::erase(bucket->second, &*found);
            if (bucket->second.empty())
                deletes.erase(bucket);
        }

        const bool wasLongest = key.size() == static_cast<std::size_t>(maxDictionaryWordLength);
        words.erase(found);
        if (wasLongest)
        {
            maxDictionaryWordLength = 0;
            for (const auto &[word, count] : words)
                maxDictionaryWordLength = std::max(maxDictionaryWordLength, static_cast<int>(word.size()));
        }
        return true;
    }

    namespace
    {
        void SplitColumns(std::u32string_view line, xchar separator, std::vector<std::u32string_view> &columns)
        {
            columns.clear();
            if (separator == DEFAULT_SEPARATOR_CHAR)
            {
                while (!line.empty())
                {
                    const auto start = std::ranges::find_if_not(line, unicode::is_space);
                    const auto end = std::find_if(start, line.end(), unicode::is_space);
                    if (start != end)
                        columns.emplace_back(start, end);
                    line.remove_prefix(end - line.begin());
                }
                return;
            }

            for (auto next = line.find(separator); next != line.npos; next = line.find(separator))
            {
                columns.push_back(line.substr(0, next));
                line.remove_prefix(next + 1);
            }
            columns.push_back(line);
        }

        std::optional<int64_t> ParseCount(std::u32string_view text)
        {
            while (!text.empty() && unicode::is_space(text.front()))
                text.remove_prefix(1);
            while (!text.empty() && unicode::is_space(text.back()))
                text.remove_suffix(1);
            if (text.starts_with(U'+'))
                text.remove_prefix(1);

            const auto bytes = unicode::encode(text);
            int64_t count;
            const auto end = bytes.data() + bytes.size();
            const auto result = std::from_chars(bytes.data(), end, count);
            if (result.ec == std::errc{} && result.ptr == end)
                return count;
            return std::nullopt;
        }

        void ValidateColumns(int termIndex, int countIndex)
        {
            if (termIndex < 0 || countIndex < 0)
                throw std::invalid_argument("Dictionary column indices cannot be negative");
        }

        int64_t AddCount(int64_t count, int64_t increment)
        {
            return count > INT64_MAX - increment ? INT64_MAX : count + increment;
        }

        int64_t EstimatedCount(double count)
        {
            if (!(count > 0))
                return 0;
            return count >= static_cast<double>(INT64_MAX) ? INT64_MAX : static_cast<int64_t>(count);
        }

        constexpr std::string_view archiveMagic = "SymSpellCppPy\x01";
    }

    bool SymSpell::LoadBigramDictionary(const std::filesystem::path &corpus, int termIndex, int countIndex,
                                         xchar separator)
    {
        std::ifstream stream(corpus, std::ios::binary);
        return stream && LoadBigramDictionary(stream, termIndex, countIndex, separator);
    }

    bool SymSpell::LoadBigramDictionary(std::istream &stream, int termIndex, int countIndex, xchar separator)
    {
        ValidateColumns(termIndex, countIndex);
        const auto term = static_cast<std::size_t>(termIndex);
        const auto countColumn = static_cast<std::size_t>(countIndex);
        const bool splitTerm = separator == DEFAULT_SEPARATOR_CHAR;
        bool loaded = false;
        std::string bytes;
        xstring line;
        std::vector<std::u32string_view> columns;
        for (bool first = true; unicode::read_line(stream, bytes, line, first); first = false)
        {
            SplitColumns(line, separator, columns);
            if (columns.size() <= std::max(term + splitTerm, countColumn) || columns[term].empty())
                continue;
            const auto count = ParseCount(columns[countColumn]);
            if (!count || *count < 0)
                continue;

            xstring key(columns[term]);
            if (splitTerm)
                key.append(1, U' ').append(columns[term + 1]);
            bigrams.insert_or_assign(std::move(key), *count);
            loaded = true;
        }
        if (loaded)
        {
            bigramCountMin = INT64_MAX;
            for (const auto &[key, count] : bigrams)
                bigramCountMin = std::min(bigramCountMin, count);
        }
        return loaded;
    }

    bool SymSpell::LoadDictionary(const std::filesystem::path &corpus, int termIndex, int countIndex,
                                   xchar separator)
    {
        std::ifstream stream(corpus, std::ios::binary);
        return stream && LoadDictionary(stream, termIndex, countIndex, separator);
    }

    bool SymSpell::LoadDictionary(std::istream &stream, int termIndex, int countIndex, xchar separator)
    {
        ValidateColumns(termIndex, countIndex);
        const auto term = static_cast<std::size_t>(termIndex);
        const auto count = static_cast<std::size_t>(countIndex);
        auto staging = std::make_shared<SuggestionStage>(16384);
        std::string bytes;
        xstring line;
        std::vector<std::u32string_view> columns;
        for (bool first = true; unicode::read_line(stream, bytes, line, first); first = false)
        {
            SplitColumns(line, separator, columns);
            if (columns.size() > term && !columns[term].empty())
            {
                const auto frequency = columns.size() > count ? ParseCount(columns[count]) : std::nullopt;
                CreateDictionaryEntry(xstring(columns[term]), frequency.value_or(1), staging);
            }
        }
        CommitStaged(staging);
        return EntryCount() != 0;
    }

    bool SymSpell::CreateDictionary(const std::filesystem::path &corpus)
    {
        std::ifstream stream(corpus, std::ios::binary);
        return stream && CreateDictionary(stream);
    }

    bool SymSpell::CreateDictionary(std::istream &stream)
    {
        auto staging = std::make_shared<SuggestionStage>(16384);
        std::string bytes;
        xstring line;
        for (bool first = true; unicode::read_line(stream, bytes, line, first); first = false)
        {
            for (const auto &key : ParseWords(line))
                CreateDictionaryEntry(key, 1, staging);
        }
        CommitStaged(staging);
        return EntryCount() != 0;
    }

    void SymSpell::Save(std::ostream &stream) const
    {
        stream.write(archiveMagic.data(), archiveMagic.size());
        cereal::PortableBinaryOutputArchive archive(stream, cereal::PortableBinaryOutputArchive::Options::LittleEndian());
        archive(initialCapacity, maxDictionaryEditDistance, prefixLength, countThreshold, compactLevel,
                maxDictionaryWordLength, belowThresholdWords, bigrams, bigramCountMin);

        // Serialize the words once and represent delete buckets by their word IDs.
        std::unordered_map<const WordCounts::value_type *, uint32_t> ids;
        ids.reserve(words.size());
        archive(static_cast<uint32_t>(words.size()));
        for (const auto &word : words)
        {
            ids.emplace(&word, static_cast<uint32_t>(ids.size()));
            archive(word.first, word.second);
        }

        archive(static_cast<uint32_t>(deletes.size()));
        std::vector<uint32_t> bucket;
        for (const auto &[hash, entries] : deletes)
        {
            bucket.clear();
            for (const auto *word : entries)
                bucket.push_back(ids.at(word));
            archive(hash, bucket);
        }
    }

    void SymSpell::Load(std::istream &stream)
    {
        std::string magic(archiveMagic.size(), '\0');
        stream.read(magic.data(), magic.size());
        if (magic == archiveMagic)
        {
            cereal::PortableBinaryInputArchive archive(stream);
            int capacity, distance, prefix, threshold;
            unsigned char compact;
            archive(capacity, distance, prefix, threshold, compact);
            SymSpell loaded(distance, prefix, threshold, capacity, compact);
            archive(loaded.maxDictionaryWordLength, loaded.belowThresholdWords, loaded.bigrams,
                    loaded.bigramCountMin);
            uint32_t wordCount;
            archive(wordCount);
            loaded.words.reserve(wordCount);
            std::vector<const WordCounts::value_type *> wordsById;
            wordsById.reserve(wordCount);
            for (uint32_t i = 0; i < wordCount; ++i)
            {
                xstring word;
                int64_t count;
                archive(word, count);
                wordsById.push_back(&*loaded.words.emplace(std::move(word), count).first);
            }

            uint32_t bucketCount;
            archive(bucketCount);
            loaded.deletes.reserve(bucketCount);
            std::vector<uint32_t> ids;
            for (uint32_t i = 0; i < bucketCount; ++i)
            {
                int hash;
                archive(hash, ids);
                auto &entries = loaded.deletes[hash];
                entries.reserve(ids.size());
                for (const auto id : ids)
                    entries.push_back(wordsById.at(id));
            }
            *this = std::move(loaded);
            return;
        }
        if (magic.starts_with("SymSpellCppPy"))
            throw std::invalid_argument("Unsupported SymSpellCppPy archive version");

        // Older releases stored UTF-8 words and byte-based delete hashes. Rebuild that index.
        stream.clear();
        stream.seekg(0);
        cereal::BinaryInputArchive archive(stream);
        std::shared_ptr<std::unordered_map<int, std::vector<std::string>>> legacyDeletes;
        std::unordered_map<std::string, int64_t> legacyWords;
        int legacyMaxLength;
        archive(legacyDeletes, legacyWords, legacyMaxLength);
        SymSpell loaded(maxDictionaryEditDistance, prefixLength, 0, initialCapacity, compactLevel);
        auto staging = std::make_shared<SuggestionStage>(16384);
        for (const auto &[word, count] : legacyWords)
            loaded.CreateDictionaryEntry(unicode::decode(word), count, staging);
        loaded.CommitStaged(staging);
        loaded.countThreshold = countThreshold;
        *this = std::move(loaded);
    }

    void SymSpell::PurgeBelowThresholdWords()
    {
        belowThresholdWords.clear();
    }

    void SymSpell::CommitStaged(const std::shared_ptr<SuggestionStage> &staging)
    {
        if (!staging || staging->DeleteCount() == 0)
            return;
        if (deletes.empty())
            deletes.reserve(staging->DeleteCount());
        staging->CommitTo(deletes, words);
    }

    std::vector<SuggestItem> SymSpell::Lookup(const xstring &input, Verbosity verbosity)
    {
        return Lookup(input, verbosity, maxDictionaryEditDistance, false, false);
    }

    std::vector<SuggestItem> SymSpell::Lookup(const xstring &input, Verbosity verbosity, int maxEditDistance)
    {
        return Lookup(input, verbosity, maxEditDistance, false, false);
    }

    std::vector<SuggestItem> SymSpell::Lookup(const xstring &input, Verbosity verbosity, int maxEditDistance, bool includeUnknown)
    {
        return Lookup(input, verbosity, maxEditDistance, includeUnknown, false);
    }

    std::vector<SuggestItem>
    SymSpell::Lookup(const xstring &original_input, Verbosity verbosity, int maxEditDistance, bool includeUnknown,
                     bool transferCasing)
    {
        if (maxEditDistance < 0)
            throw std::invalid_argument("Distance cannot be negative");
        if (maxEditDistance > maxDictionaryEditDistance)
            throw std::invalid_argument("Distance too large");

        if (words.empty())
        {
            if (includeUnknown)
                return {{original_input, maxEditDistance + 1, 0}};
            return {};
        }

        bool skip = false;
        xstring lower_input;

        if (transferCasing)
        {
            lower_input = Helpers::string_lower(original_input);
        }

        const xstring &input = transferCasing ? lower_input : original_input;

        std::vector<SuggestItem> suggestions;
        int inputLen = input.size();
        if (inputLen - maxEditDistance > maxDictionaryWordLength)
            skip = true;

        int64_t suggestionCount = 0;
        if (const auto exact = words.find(input); exact != words.end() && !skip)
        {
            suggestionCount = exact->second;
            suggestions.emplace_back(transferCasing ? original_input : input, 0, suggestionCount);
            if (verbosity != All)
                skip = true;
        }

        if (maxEditDistance == 0)
            skip = true;

        if (!skip)
        {
            std::unordered_set<xstring> hashset1;
            std::unordered_set<const WordCounts::value_type *> seenSuggestions;

            int maxEditDistance2 = maxEditDistance;
            int candidatePointer = 0;
            std::deque<xstring> candidates;

            int inputPrefixLen = inputLen;
            if (inputPrefixLen > prefixLength)
            {
                inputPrefixLen = prefixLength;
                candidates.emplace_back(input.substr(0, inputPrefixLen));
            }
            else
            {
                candidates.emplace_back(input);
            }
            auto distanceComparer = EditDistance(distanceAlgorithm);
            while (candidatePointer < candidates.size())
            {
                const xstring &candidate = candidates[candidatePointer++];
                int candidateLen = candidate.size();
                int lengthDiff = inputPrefixLen - candidateLen;

                if (lengthDiff > maxEditDistance2)
                {
                    if (verbosity == Verbosity::All)
                        continue;
                    break;
                }

                auto deletes_found = deletes.find(GetstringHash(candidate));

                // read candidate entry from std::unordered_map
                if (deletes_found != deletes.end())
                {
                    for (const auto *word : deletes_found->second)
                    {
                        const auto &suggestion = word->first;
                        int suggestionLen = suggestion.size();
                        if (suggestion == input)
                            continue;
                        if ((abs(suggestionLen - inputLen) >
                             maxEditDistance2) // input and sugg lengths diff > allowed/current best distance
                            || (suggestionLen <
                                candidateLen) // sugg must be for a different delete string, in same bin only because of hash collision
                            || (suggestionLen == candidateLen && suggestion !=
                                                                     candidate)) // if sugg len = delete len, then it either equals delete or is in same bin only because of hash collision
                            continue;
                        auto suggPrefixLen = std::min(suggestionLen, prefixLength);
                        if (suggPrefixLen > inputPrefixLen &&
                            (suggPrefixLen - candidateLen) > maxEditDistance2)
                            continue;

                        int distance = 0;
                        int min_len = 0;
                        if (candidateLen == 0)
                        {
                            // suggestions which have no common chars with input (inputLen<=maxEditDistance && suggestionLen<=maxEditDistance)
                            distance = std::max(inputLen, suggestionLen);
                            auto flag = seenSuggestions.insert(word);
                            if (distance > maxEditDistance2 || !flag.second)
                                continue;
                        }
                        else if (suggestionLen == 1)
                        {
                            if (input.find(suggestion[0]) == xstring::npos)
                                distance = inputLen;
                            else
                                distance = inputLen - 1;

                            auto flag = seenSuggestions.insert(word);
                            if (distance > maxEditDistance2 || !flag.second)
                                continue;
                        }
                        else
                        {
                            // A delete using the whole budget in the prefix cannot also change the suffix.
                            const int suffixLength = std::min(inputLen, suggestionLen) - prefixLength;
                            if (candidateLen == prefixLength - maxEditDistance && suffixLength > 0)
                            {
                                const int inputStart = inputLen - suffixLength;
                                const int suggestionStart = suggestionLen - suffixLength;
                                const bool tailsDiffer = std::u32string_view(input).substr(inputStart + 1) !=
                                    std::u32string_view(suggestion).substr(suggestionStart + 1);
                                const bool boundaryDiffers = input[inputStart] != suggestion[suggestionStart] &&
                                    (input[inputStart - 1] != suggestion[suggestionStart] ||
                                     input[inputStart] != suggestion[suggestionStart - 1]);
                                if (tailsDiffer || boundaryDiffers)
                                    continue;
                            }

                            if ((verbosity != All &&
                                 !DeleteInSuggestionPrefix(candidate, candidateLen, suggestion, suggestionLen)) ||
                                !seenSuggestions.insert(word).second)
                                continue;
                            distance = distanceComparer.Compare(input, suggestion, maxEditDistance2);
                            if (distance < 0)
                                continue;
                        }

                        if (distance <= maxEditDistance2)
                        {
                            suggestionCount = word->second;
                            SuggestItem si = SuggestItem(suggestion, distance, suggestionCount);
                            if (!suggestions.empty())
                            {
                                switch (verbosity)
                                {
                                case Closest:
                                {
                                    if (distance < maxEditDistance2)
                                        suggestions.clear();
                                    break;
                                }
                                case Top:
                                {
                                    if (distance < maxEditDistance2 || suggestionCount > suggestions[0].count)
                                    {
                                        maxEditDistance2 = distance;
                                        suggestions[0] = si;
                                    }
                                    continue;
                                }
                                case All:
                                    break;
                                }
                            }
                            if (verbosity != All)
                                maxEditDistance2 = distance;
                            suggestions.push_back(si);
                        }
                    } // end foreach
                }     // end if

                if ((lengthDiff < maxEditDistance) && (candidateLen <= prefixLength))
                {
                    if (verbosity != All && lengthDiff >= maxEditDistance2)
                        continue;

                    for (int i = 0; i < candidateLen; i++)
                    {
                        xstring del(candidate);
                        del.erase(i, 1);

                        if (hashset1.insert(del).second)
                        {
                            candidates.push_back(del);
                        }
                    }
                }
            } // end while

            if (suggestions.size() > 1)
                std::sort(suggestions.begin(), suggestions.end(), SuggestItem::compare);

            if (transferCasing)
            {
                for (auto &suggestion : suggestions)
                {
                    suggestion.term = Helpers::transfer_casing_for_similar_text(original_input, suggestion.term);
                }
            }
        }
        if (includeUnknown && (suggestions.empty()))
            suggestions.emplace_back(original_input, maxEditDistance + 1, 0);
        return suggestions;
    } // end if

    bool SymSpell::DeleteInSuggestionPrefix(const xstring &deleteSugg, int deleteLen, const xstring &suggestion,
                                            int suggestionLen) const
    {
        if (deleteLen == 0)
            return true;
        if (prefixLength < suggestionLen)
            suggestionLen = prefixLength;
        int j = 0;
        for (int i = 0; i < deleteLen; i++)
        {
            xchar delChar = deleteSugg[i];
            while (j < suggestionLen && delChar != suggestion[j])
                j++;
            if (j == suggestionLen)
                return false;
            ++j;
        }
        return true;
    }

    std::vector<xstring> SymSpell::ParseWords(const xstring &text)
    {
        const auto isTermCharacter = [](xchar ch) {
            return unicode::is_word(ch) || ch == U'\'' || ch == U'’' ||
                   ch == U'-' || ch == U'[' || ch == U']';
        };
        std::vector<xstring> matches;
        for (auto start = text.begin(); start != text.end();)
        {
            start = std::find_if(start, text.end(), isTermCharacter);
            const auto end = std::find_if_not(start, text.end(), isTermCharacter);
            if (start != end)
                matches.push_back(Helpers::string_lower(xstring(start, end)));
            start = end;
        }
        return matches;
    }

    std::shared_ptr<std::unordered_set<xstring>>
    SymSpell::Edits(const xstring &word, int editDistance, std::shared_ptr<std::unordered_set<xstring>> deleteWords)
    {
        editDistance++;
        if (word.size() > 1)
        {
            for (int i = 0; i < word.size(); i++)
            {
                xstring temp(word);
                xstring del = temp.erase(i, 1);
                if (deleteWords->insert(del).second)
                {
                    if (editDistance < maxDictionaryEditDistance)
                        Edits(del, editDistance, deleteWords);
                }
            }
        }
        return deleteWords;
    }

    std::shared_ptr<std::unordered_set<xstring>> SymSpell::EditsPrefix(const xstring &key)
    {
        auto m = std::make_shared<std::unordered_set<xstring>>();
        if (key.size() <= maxDictionaryEditDistance)
            m->insert(XL(""));
        if (key.size() > prefixLength)
        {
            const xstring sub_key = key.substr(0, prefixLength);
            m->insert(sub_key);
            if (maxDictionaryEditDistance > 0)
                Edits(sub_key, 0, m);
        }
        else
        {
            m->insert(key);
            if (maxDictionaryEditDistance > 0)
                Edits(key, 0, m);
        }
        return m;
    }

    int SymSpell::GetstringHash(const xstring &s) const
    {
        int len = s.size();
        int lenMask = len;
        if (lenMask > 3)
            lenMask = 3;

        unsigned int hash = 2166136261;
        for (auto i = 0; i < len; i++)
        {
            hash ^= s[i];
            hash *= 16777619;
        }

        hash &= compactMask;
        hash |= (unsigned int)lenMask;
        return (int)hash;
    }

    std::vector<SuggestItem> SymSpell::LookupCompound(const xstring &input)
    {
        return LookupCompound(input, maxDictionaryEditDistance, false);
    }

    std::vector<SuggestItem> SymSpell::LookupCompound(const xstring &input, int editDistanceMax)
    {
        return LookupCompound(input, editDistanceMax, false);
    }

    std::vector<SuggestItem> SymSpell::LookupCompound(const xstring &input, int editDistanceMax, bool transferCasing)
    {
        if (editDistanceMax < 0)
            throw std::invalid_argument("Distance cannot be negative");
        if (editDistanceMax > maxDictionaryEditDistance)
            throw std::invalid_argument("Distance too large");

        std::vector<xstring> termList1 = ParseWords(input);

        std::vector<SuggestItem> suggestions;     // suggestions for a single term
        std::vector<SuggestItem> suggestionParts; // 1 line with separate parts
        auto distanceComparer = EditDistance(distanceAlgorithm);

        bool lastCombi = false;
        for (int i = 0; i < termList1.size(); i++)
        {
            suggestions = Lookup(termList1[i], Top, editDistanceMax);

            if ((i > 0) && !lastCombi)
            {
                std::vector<SuggestItem> suggestionsCombi = Lookup(termList1[i - 1] + termList1[i], Top,
                                                                   editDistanceMax);

                if (!suggestionsCombi.empty())
                {
                    SuggestItem best1 = suggestionParts[suggestionParts.size() - 1];
                    SuggestItem best2 = SuggestItem();
                    if (!suggestions.empty())
                    {
                        best2 = suggestions[0];
                    }
                    else
                    {
                        best2.term = termList1[i];
                        best2.distance = editDistanceMax + 1;
                        best2.count = (int64_t)((double)10 / pow((double)10, (double)best2.term.size())); // 0;
                    }

                    int distance1 = best1.distance + best2.distance;
                    if ((distance1 >= 0) && ((suggestionsCombi[0].distance + 1 < distance1) ||
                                             ((suggestionsCombi[0].distance + 1 == distance1) &&
                                              ((double)suggestionsCombi[0].count >
                                               (double)best1.count / (double)N * (double)best2.count))))
                    {
                        suggestionsCombi[0].distance++;
                        suggestionParts[suggestionParts.size() - 1] = suggestionsCombi[0];
                        lastCombi = true;
                        goto nextTerm;
                    }
                }
            }
            lastCombi = false;

            if ((!suggestions.empty()) && ((suggestions[0].distance == 0) || (termList1[i].size() == 1)))
            {
                suggestionParts.push_back(suggestions[0]);
            }
            else
            {
                SuggestItem suggestionSplitBest;

                if (!suggestions.empty())
                    suggestionSplitBest.set(suggestions[0]);

                if (termList1[i].size() > 1)
                {
                    for (int j = 1; j < termList1[i].size(); j++)
                    {
                        xstring part1 = termList1[i].substr(0, j);
                        xstring part2 = termList1[i].substr(j);
                        SuggestItem suggestionSplit = SuggestItem();
                        std::vector<SuggestItem> suggestions1 = Lookup(part1, Top, editDistanceMax);
                        if (!suggestions1.empty())
                        {
                            std::vector<SuggestItem> suggestions2 = Lookup(part2, Top, editDistanceMax);
                            if (!suggestions2.empty())
                            {
                                suggestionSplit.term = suggestions1[0].term + XL(" ") + suggestions2[0].term;

                                int distance2 = distanceComparer.Compare(termList1[i], suggestionSplit.term,
                                                                         editDistanceMax);
                                if (distance2 < 0)
                                    distance2 = editDistanceMax + 1;

                                if (suggestionSplitBest.count)
                                {
                                    if (distance2 > suggestionSplitBest.distance)
                                        continue;
                                    if (distance2 < suggestionSplitBest.distance)
                                        suggestionSplitBest.count = 0;
                                }

                                suggestionSplit.distance = distance2;
                                if (bigrams.count(suggestionSplit.term) > 0)
                                {
                                    int64_t bigramCount = bigrams.at(suggestionSplit.term);
                                    suggestionSplit.count = bigramCount;
                                    if (!suggestions.empty())
                                    {
                                        if ((suggestions1[0].term + suggestions2[0].term == termList1[i]))
                                        {
                                            suggestionSplit.count = std::max(suggestionSplit.count,
                                                                             AddCount(suggestions[0].count, 2));
                                        }
                                        else if ((suggestions1[0].term == suggestions[0].term) ||
                                                 (suggestions2[0].term == suggestions[0].term))
                                        {
                                            suggestionSplit.count = std::max(suggestionSplit.count,
                                                                             AddCount(suggestions[0].count, 1));
                                        }
                                    }
                                    else if ((suggestions1[0].term + suggestions2[0].term == termList1[i]))
                                    {
                                        suggestionSplit.count = std::max(suggestionSplit.count,
                                                                         AddCount(std::max(suggestions1[0].count, suggestions2[0].count), 2));
                                    }
                                }
                                else
                                {
                                    suggestionSplit.count = std::min(bigramCountMin,
                                                                     EstimatedCount(static_cast<double>(suggestions1[0].count) /
                                                                                    N * suggestions2[0].count));
                                }

                                if (suggestionSplitBest.count == 0 ||
                                    (suggestionSplit.count > suggestionSplitBest.count))
                                    suggestionSplitBest.set(suggestionSplit);
                            }
                        }
                    }

                    if (suggestionSplitBest.count)
                    {
                        suggestionParts.push_back(suggestionSplitBest);
                    }
                    else
                    {
                        SuggestItem si = SuggestItem();
                        si.term = termList1[i];
                        si.count = (int64_t)((double)10 / pow((double)10, (double)si.term.size()));
                        si.distance = editDistanceMax + 1;
                        suggestionParts.push_back(si);
                    }
                }
                else
                {
                    SuggestItem si = SuggestItem();
                    si.term = termList1[i];
                    si.count = (int64_t)((double)10 / pow((double)10, (double)si.term.size()));
                    si.distance = editDistanceMax + 1;
                    suggestionParts.push_back(si);
                }
            }
        nextTerm:;
        }

        double count = N;
        xstring s;
        for (const SuggestItem &si : suggestionParts)
        {
            if (!s.empty())
                s += U' ';
            s += si.term;
            count *= (double)si.count / (double)N;
        }
        if (transferCasing)
        {
            s = Helpers::transfer_casing_for_similar_text(input, s);
        }
        std::vector<SuggestItem> suggestionsLine;
        suggestionsLine.emplace_back(s, distanceComparer.Compare(input, s, INT64_MAX), EstimatedCount(count));
        return suggestionsLine;
    }

    Info SymSpell::WordSegmentation(const xstring &input)
    {
        return WordSegmentation(input, MaxDictionaryEditDistance());
    }

    Info SymSpell::WordSegmentation(const xstring &input, int maxEditDistance)
    {
        const auto wordLength = maxDictionaryWordLength > 0 ? maxDictionaryWordLength : static_cast<int>(input.size());
        return WordSegmentation(input, maxEditDistance, std::max(1, wordLength));
    }

    Info SymSpell::WordSegmentation(const xstring &originalInput, int maxEditDistance, int maxSegmentationWordLength)
    {
        if (maxEditDistance < 0)
            throw std::invalid_argument("Distance cannot be negative");
        if (maxEditDistance > maxDictionaryEditDistance)
            throw std::invalid_argument("Distance too large");
        if (maxSegmentationWordLength <= 0)
            throw std::invalid_argument("max_segmentation_word_length must be positive");

        auto input = unicode::normalize(originalInput);
        std::erase(input, U'-');
        if (std::ranges::all_of(input, unicode::is_space))
            return {};

        const auto arraySize = std::min(maxSegmentationWordLength, static_cast<int>(input.size()));
        std::vector<Info> compositions(arraySize);
        std::vector<int> endPositions(arraySize, -1);
        int circularIndex = arraySize - 1;
        const auto isApostrophe = [](xchar ch) { return ch == U'\'' || ch == U'’'; };

        for (int j = 0; j < static_cast<int>(input.size()); ++j)
        {
            const auto limit = std::min(static_cast<int>(input.size()) - j, maxSegmentationWordLength);
            for (int i = 1; i <= limit; ++i)
            {
                auto part = input.substr(j, i);
                const int separatorLength = unicode::is_space(part.front()) ? 0 : 1;
                if (separatorLength == 0)
                    part.erase(0, 1);
                const int removedSpaces = std::erase_if(part, unicode::is_space);

                const bool punctuation = part.size() == 1 && unicode::is_punctuation(part.front());
                const bool apostropheSuffix = part.size() == 2 && isApostrophe(part.front());
                if (part.size() > 1 && std::ranges::any_of(part, [&](xchar ch) {
                        return !isApostrophe(ch) && unicode::is_punctuation(ch);
                    }))
                    continue;

                int distance = removedSpaces;
                double logProbability = 0;
                xstring corrected = part;
                if (!part.empty() && !punctuation && !apostropheSuffix)
                {
                    const auto apostrophes = std::ranges::count_if(part, isApostrophe);
                    auto lookup = Helpers::string_lower(part);
                    std::ranges::replace(lookup, U'’', U'\'');
                    const auto results = Lookup(lookup, Top, maxEditDistance);
                    if (!results.empty() && std::ranges::count_if(results.front().term, isApostrophe) == apostrophes)
                    {
                        corrected = results.front().term;
                        if (!corrected.empty() && unicode::is_upper(part.front()))
                            corrected.front() = unicode::upper(corrected.front());
                        if (part.find(U'’') != part.npos)
                            std::ranges::replace(corrected, U'\'', U'’');
                        distance += results.front().distance;
                        logProbability = std::log10(static_cast<double>(results.front().count) / N);
                    }
                    else
                    {
                        distance += static_cast<int>(part.size());
                        logProbability = 1 - std::log10(static_cast<double>(N)) - part.size();
                    }
                }

                const auto destinationIndex = (i + circularIndex) % arraySize;
                auto &destination = compositions[destinationIndex];
                if (j == 0)
                {
                    destination.set(std::move(part), std::move(corrected), distance, logProbability);
                    endPositions[destinationIndex] = j + i;
                    continue;
                }

                const auto &previous = compositions[circularIndex];
                const auto previousDistance = previous.getDistance();
                const auto combinedProbability = previous.getProbability() + logProbability;
                const auto unspacedDistance = previousDistance + distance;
                const bool attach = corrected.empty() || punctuation || apostropheSuffix;
                const bool addSpace = !attach && !previous.getSegmented().empty();
                const auto spacedDistance = unspacedDistance + (addSpace ? separatorLength : 0);

                if (endPositions[destinationIndex] != j + i || spacedDistance < destination.getDistance() ||
                    ((unspacedDistance == destination.getDistance() || spacedDistance == destination.getDistance()) &&
                     destination.getProbability() < combinedProbability))
                {
                    auto segmented = previous.getSegmented();
                    auto correctedString = previous.getCorrected();
                    if (addSpace)
                    {
                        segmented += U' ';
                        correctedString += U' ';
                    }
                    segmented += part;
                    correctedString += corrected;
                    destination.set(std::move(segmented), std::move(correctedString), spacedDistance, combinedProbability);
                    endPositions[destinationIndex] = j + i;
                }
            }
            circularIndex = (circularIndex + 1) % arraySize;
        }
        return compositions[circularIndex];
    }

}
