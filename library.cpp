#include "library.h"

#include <codecvt>
#include <utility>

namespace symspellcpppy {

    int SymSpell::MaxDictionaryEditDistance() const {
        return this->maxDictionaryEditDistance;
    }

    int SymSpell::PrefixLength() const {
        return this->prefixLength;
    }

    int SymSpell::MaxLength() const {
        return this->maxDictionaryWordLength;
    }

    long SymSpell::CountThreshold() const {
        return this->countThreshold;
    }

    int SymSpell::WordCount() {
        return this->words.size();
    }

    int SymSpell::EntryCount() {
        return this->deletes->size();
    }

    SymSpell::SymSpell(int initialCapacity, int maxDictionaryEditDistance, int prefixLength, int countThreshold,
                       unsigned char compactLevel) {
        if (initialCapacity < 0) throw std::invalid_argument("initialCapacity");
        if (maxDictionaryEditDistance < 0) throw std::invalid_argument("maxDictionaryEditDistance");
        if (prefixLength < 1 || prefixLength <= maxDictionaryEditDistance) throw std::invalid_argument("prefixLength");
        if (countThreshold < 0) throw std::invalid_argument("countThreshold");
        if (compactLevel > 16) throw std::invalid_argument("compactLevel");

        this->initialCapacity = initialCapacity;
        this->words.reserve(initialCapacity);
        this->maxDictionaryEditDistance = maxDictionaryEditDistance;
        this->prefixLength = prefixLength;
        this->countThreshold = countThreshold;
        if (compactLevel > 16) compactLevel = 16;
        this->compactMask = (UINT_MAX >> (3 + compactLevel)) << 2;
        this->maxDictionaryWordLength = 0;
        this->words = std::unordered_map<xstring, int64_t>(initialCapacity);
    }

    bool SymSpell::CreateDictionaryEntry(const xstring &key, int64_t count, SuggestionStage *staging) {

        if (count <= 0) {
            if (this->countThreshold > 0)
                return false; // no point doing anything if count is zero, as it can't change anything
            count = 0;
        }
        int countPrevious = -1;
        auto belowThresholdWordsFinded = belowThresholdWords.find(key);
        auto wordsFinded = words.find(key);
        if (countThreshold > 1 && belowThresholdWordsFinded != belowThresholdWords.end()) {
            countPrevious = belowThresholdWordsFinded->second;
            count = (MAXINT - countPrevious > count) ? countPrevious + count : MAXINT;
            if (count >= countThreshold) {
                belowThresholdWords.erase(key);
            } else {
                belowThresholdWords.insert(std::pair<xstring, int64_t>(key, count));
                return false;
            }
        } else if (wordsFinded != words.end()) {
            countPrevious = wordsFinded->second;
            count = (MAXINT - countPrevious > count) ? countPrevious + count : MAXINT;
            words.insert(std::pair<xstring, int64_t>(key, count));
            return false;
        } else if (count < CountThreshold()) {
            belowThresholdWords.insert(std::pair<xstring, int64_t>(key, count));
            return false;
        }

        words.insert(std::pair<xstring, int64_t>(key, count));

        if (key.size() > this->maxDictionaryWordLength) this->maxDictionaryWordLength = key.size();

        //create deletes
        std::unordered_set<xstring> edits = EditsPrefix(key);
        if (staging != nullptr) {
            for (const auto &edit : edits) {
                staging->Add(GetstringHash(edit), key);
            }
        } else {

            for (const auto &edit : edits) {
                int deleteHash = GetstringHash(edit);
                auto deletesFinded = deletes->find(deleteHash);
                std::vector<xstring> suggestions;
                if (deletesFinded != deletes->end()) {
                    suggestions = deletesFinded->second;
                    std::vector<xstring> newSuggestions(suggestions.size() + 1);
                    for (int id = 0; id < suggestions.size(); id++) {
                        newSuggestions[id] = suggestions[id];
                    }

                    (*deletes)[deleteHash] = suggestions = newSuggestions;
                } else {
                    suggestions = std::vector<xstring>(1);
                    (*deletes).insert(std::pair<int, std::vector<xstring>>(deleteHash, suggestions));
                }
                suggestions[suggestions.size() - 1] = key;
            }

        }

        return true;
    }

    bool
    SymSpell::LoadBigramDictionary(const std::string &corpus, int termIndex, int countIndex, xchar separatorChars) {
        xifstream corpusStream;
        corpusStream.open(corpus);
#ifdef UNICODE_SUPPORT
        std::locale utf8(std::locale(), new std::codecvt_utf8<wchar_t>);
        corpusStream.imbue(utf8);
#endif
        if (!corpusStream.is_open())
            return false;

        return LoadBigramDictionary(corpusStream, termIndex, countIndex, separatorChars);
    }

    bool SymSpell::LoadBigramDictionary(xifstream &corpusStream, int termIndex, int countIndex, xchar separatorChars) {
        xstring line;
        int linePartsLength = (separatorChars == DEFAULT_SEPARATOR_CHAR) ? 3 : 2;
        while (getline(corpusStream, line)) {
            std::vector<xstring> lineParts;
            xstringstream ss(line);
            xstring token;
            while (getline(ss, token, separatorChars))
                lineParts.push_back(token);
            xstring key;
            int64_t count;
            if (lineParts.size() >= linePartsLength) {
                key = (separatorChars == DEFAULT_SEPARATOR_CHAR) ? lineParts[termIndex] + XL(" ") +
                                                                   lineParts[termIndex + 1]
                                                                 : lineParts[termIndex];
                try {
                    count = stoll(lineParts[countIndex]);

                } catch (...) {
                    printf("Cannot convert %ls to integer\n", lineParts[countIndex].c_str());
                }
            } else {
                key = line;
                count = 1;
            }
            std::pair<xstring, int64_t> element(key, count);
            bigrams.insert(element);
            if (count < bigramCountMin) bigramCountMin = count;
        }

        if (bigrams.empty())
            return false;
        return true;
    }

    bool SymSpell::LoadDictionary(const std::string &corpus, int termIndex, int countIndex, xchar separatorChars) {

        xifstream corpusStream(corpus);
#ifdef UNICODE_SUPPORT
        std::locale utf8(std::locale(), new std::codecvt_utf8<wchar_t>);
        corpusStream.imbue(utf8);
#endif
        if (!corpusStream.is_open())
            return false;

        return LoadDictionary(corpusStream, termIndex, countIndex, separatorChars);
    }

    bool SymSpell::LoadDictionary(xifstream &corpusStream, int termIndex, int countIndex, xchar separatorChars) {
        SuggestionStage staging(16384);
        xstring line;
        int i = 0;
        int start, end;
        start = clock();
        while (getline(corpusStream, line)) {
            i++;
            std::vector<xstring> lineParts;
            xstringstream ss(line);
            xstring token;
            while (getline(ss, token, separatorChars))
                lineParts.push_back(token);
            if (lineParts.size() >= 2) {
                int64_t count = stoll(lineParts[countIndex]);

                CreateDictionaryEntry(lineParts[termIndex], count, &staging);
            } else {
                CreateDictionaryEntry(line, 1, &staging);
            }

        }
        if (this->deletes == nullptr)
            this->deletes = new std::unordered_map<int, std::vector<xstring>>(staging.DeleteCount());
        CommitStaged(&staging);
        if (this->EntryCount() == 0)
            return false;
        return true;
    }

    bool SymSpell::CreateDictionary(const std::string &corpus) {
        xifstream corpusStream;
        corpusStream.open(corpus);
#ifdef UNICODE_SUPPORT
        std::locale utf8(std::locale(), new std::codecvt_utf8<wchar_t>);
        corpusStream.imbue(utf8);
#endif
        if (!corpusStream.is_open()) return false;

        return CreateDictionary(corpusStream);
    }

    bool SymSpell::CreateDictionary(xifstream &corpusStream) {
        xstring line;
        SuggestionStage staging = SuggestionStage(16384);
        while (getline(corpusStream, line)) {
            for (const xstring &key : ParseWords(line)) {
                CreateDictionaryEntry(key, 1, &staging);
            }

        }
        if (this->deletes == nullptr)
            this->deletes = new std::unordered_map<int, std::vector<xstring>>(staging.DeleteCount());
        CommitStaged(&staging);
        if (this->EntryCount() == 0)
            return false;
        return true;
    }

    void SymSpell::PurgeBelowThresholdWords() {
        belowThresholdWords.clear();
    }

    void SymSpell::CommitStaged(SuggestionStage *staging) {
        staging->CommitTo(deletes);
    }

    std::vector<SuggestItem> SymSpell::Lookup(xstring input, Verbosity verbosity) {
        return Lookup(std::move(input), verbosity, this->maxDictionaryEditDistance, false);
    }

    std::vector<SuggestItem> SymSpell::Lookup(xstring input, Verbosity verbosity, int maxEditDistance) {
        return Lookup(std::move(input), verbosity, maxEditDistance, false);
    }

    std::vector<SuggestItem>
    SymSpell::Lookup(xstring input, Verbosity verbosity, int maxEditDistance, bool includeUnknown) {
        int skip = 0;
        if (maxEditDistance > this->maxDictionaryEditDistance) throw std::invalid_argument("maxEditDistance");

        std::vector<SuggestItem> suggestions;
        int inputLen = input.size();
        if (inputLen - maxEditDistance > this->maxDictionaryWordLength) skip = 1;

        int64_t suggestionCount = 0;
        if (words.count(input) && !skip) {
            suggestionCount = words.at(input);
            suggestions.emplace_back(input, 0, suggestionCount);
            if (verbosity != All) skip = 1;
        }

        if (maxEditDistance == 0) skip = 1;

        if (!skip) {
            std::unordered_set<xstring> hashset1;
            std::unordered_set<xstring> hashset2;
            hashset2.insert(input);

            int maxEditDistance2 = maxEditDistance;
            int candidatePointer = 0;
            std::vector<xstring> singleSuggestion = {XL("")};
            std::vector<xstring> candidates;

            int inputPrefixLen = inputLen;
            if (inputPrefixLen > prefixLength) {
                inputPrefixLen = prefixLength;
                candidates.push_back(input.substr(0, inputPrefixLen));
            } else {
                candidates.push_back(input);
            }
            auto distanceComparer = EditDistance(this->distanceAlgorithm);
            while (candidatePointer < candidates.size()) {
                xstring candidate = candidates[candidatePointer++];
                int candidateLen = candidate.size();
                int lengthDiff = inputPrefixLen - candidateLen;

                if (lengthDiff > maxEditDistance2) {
                    if (verbosity == Verbosity::All) continue;
                    break;
                }

                //read candidate entry from std::unordered_map
                if (deletes->count(GetstringHash(candidate))) {
                    std::vector<xstring> dictSuggestions = deletes->at(GetstringHash(candidate));
                    for (auto suggestion : dictSuggestions) {
                        int suggestionLen = suggestion.size();
                        if (suggestion == input) continue;
                        if ((abs(suggestionLen - inputLen) >
                             maxEditDistance2) // input and sugg lengths diff > allowed/current best distance
                            || (suggestionLen <
                                candidateLen) // sugg must be for a different delete string, in same bin only because of hash collision
                            || (suggestionLen == candidateLen && suggestion !=
                                                                 candidate)) // if sugg len = delete len, then it either equals delete or is in same bin only because of hash collision
                            continue;
                        auto suggPrefixLen = fmin(suggestionLen, prefixLength);
                        if (suggPrefixLen > inputPrefixLen &&
                            (suggPrefixLen - candidateLen) > maxEditDistance2)
                            continue;

                        int distance = 0;
                        int min_len = 0;
                        if (candidateLen == 0) {
                            //suggestions which have no common chars with input (inputLen<=maxEditDistance && suggestionLen<=maxEditDistance)
                            distance = fmax(inputLen, suggestionLen);
                            auto flag = hashset2.insert(suggestion);
                            if (distance > maxEditDistance2 || !flag.second) continue;
                        } else if (suggestionLen == 1) {
                            if (input.find(suggestion[0]) == xstring::npos)
                                distance = inputLen;
                            else
                                distance = inputLen - 1;

                            auto flag = hashset2.insert(suggestion);
                            if (distance > maxEditDistance2 || !flag.second) continue;
                        } else if ((prefixLength - maxEditDistance == candidateLen)
                                   && (((min_len = fmin(inputLen, suggestionLen) - prefixLength) > 1)
                                       && (input.substr(inputLen + 1 - min_len) !=
                                           suggestion.substr(suggestionLen + 1 - min_len)))
                                   ||
                                   ((min_len > 0) && (input[inputLen - min_len] != suggestion[suggestionLen - min_len])
                                    && ((input[inputLen - min_len - 1] != suggestion[suggestionLen - min_len])
                                        || (input[inputLen - min_len] != suggestion[suggestionLen - min_len - 1])))) {
                            continue;
                        } else {
                            if ((verbosity != All &&
                                 !DeleteInSuggestionPrefix(candidate, candidateLen, suggestion, suggestionLen))
                                || !hashset2.insert(suggestion).second)
                                continue;
                            distance = distanceComparer.Compare(input, suggestion, maxEditDistance2);
                            if (distance < 0) continue;
                        }

                        if (distance <= maxEditDistance2) {
                            suggestionCount = words[suggestion];
                            SuggestItem si = SuggestItem(suggestion, distance, suggestionCount);
                            if (!suggestions.empty()) {
                                switch (verbosity) {
                                    case Closest: {
                                        if (distance < maxEditDistance2) suggestions.clear();
                                        break;
                                    }
                                    case Top: {
                                        if (distance < maxEditDistance2 || suggestionCount > suggestions[0].count) {
                                            maxEditDistance2 = distance;
                                            suggestions[0] = si;
                                        }
                                        continue;
                                    }
                                    case All:
                                        break;
                                }
                            }
                            if (verbosity != All) maxEditDistance2 = distance;
                            suggestions.push_back(si);
                        }
                    }//end foreach
                }//end if

                if ((lengthDiff < maxEditDistance) && (candidateLen <= prefixLength)) {
                    if (verbosity != All && lengthDiff >= maxEditDistance2) continue;

                    for (int i = 0; i < candidateLen; i++) {
                        xstring temp(candidate);
                        xstring del = temp.erase(i, 1);

                        if (hashset1.insert(del).second) { candidates.push_back(del); }
                    }
                }
            }//end while

            if (suggestions.size() > 1)
                sort(suggestions.begin(), suggestions.end(), [](SuggestItem &l, SuggestItem &r) {
                    return l.CompareTo(r) < 0 ? 1 : 0;
                });
        }
        if (includeUnknown && (suggestions.empty())) suggestions.emplace_back(input, maxEditDistance + 1, 0);
        return suggestions;
    }//end if

    std::vector<xstring> SymSpell::LookupTerm(xstring input, Verbosity verbosity) {
        auto results = Lookup(std::move(input), verbosity, this->maxDictionaryEditDistance, false);
        std::vector<xstring> terms;
        std::transform(results.begin(), results.end(), std::back_inserter(terms), [](const SuggestItem& item) {
            return item.term;
        });
        return terms;
    }

    std::vector<xstring> SymSpell::LookupTerm(xstring input, Verbosity verbosity, int maxEditDistance) {
        auto results = Lookup(std::move(input), verbosity, maxEditDistance, false);
        std::vector<xstring> terms;
        std::transform(results.begin(), results.end(), std::back_inserter(terms), [](const SuggestItem& item) {
            return item.term;
        });
        return terms;
    }

    std::vector<xstring> SymSpell::LookupTerm(xstring input, Verbosity verbosity, int maxEditDistance, bool includeUnknown) {
        auto results = Lookup(std::move(input), verbosity, maxEditDistance, includeUnknown);
        std::vector<xstring> terms;
        std::transform(results.begin(), results.end(), std::back_inserter(terms), [](const SuggestItem& item) {
            return item.term;
        });
        return terms;
    }

    bool SymSpell::DeleteInSuggestionPrefix(xstring deleteSugg, int deleteLen, xstring suggestion,
                                            int suggestionLen) const {
        if (deleteLen == 0) return true;
        if (prefixLength < suggestionLen) suggestionLen = prefixLength;
        int j = 0;
        for (int i = 0; i < deleteLen; i++) {
            xchar delChar = deleteSugg[i];
            while (j < suggestionLen && delChar != suggestion[j]) j++;
            if (j == suggestionLen) return false;
        }
        return true;
    }

    std::vector<xstring> SymSpell::ParseWords(const xstring &text) {
        xregex r(XL("['’\\w-\\[_\\]]+"));
        xsmatch m;
        std::vector<xstring> matches;
        xstring::const_iterator ptr(text.cbegin());
        while (regex_search(ptr, text.cend(), m, r)) {
            matches.push_back(m[0]);
            ptr = m.suffix().first;
        }
        return matches;
    }

    std::unordered_set<xstring> *
    SymSpell::Edits(const xstring &word, int editDistance, std::unordered_set<xstring> *deleteWords) {
        editDistance++;
        if (word.size() > 1) {
            for (int i = 0; i < word.size(); i++) {
                xstring temp(word);
                xstring del = temp.erase(i, 1);
                if (deleteWords->insert(del).second) {
                    if (editDistance < maxDictionaryEditDistance) Edits(del, editDistance, deleteWords);
                }
            }
        }
        return deleteWords;
    }

    std::unordered_set<xstring> SymSpell::EditsPrefix(xstring key) {
        std::unordered_set<xstring> m = std::unordered_set<xstring>();
        if (key.size() <= maxDictionaryEditDistance) m.insert(XL(""));
        if (key.size() > prefixLength) key = key.substr(0, prefixLength);
        m.insert(key);
        Edits(key, 0, &m);
        return m;
    }

    int SymSpell::GetstringHash(xstring s) const {
        int len = s.size();
        int lenMask = len;
        if (lenMask > 3) lenMask = 3;

        uint hash = 2166136261;
        for (auto i = 0; i < len; i++) {
            {
                hash ^= s[i];
                hash *= 16777619;
            }
        }

        hash &= this->compactMask;
        hash |= (uint) lenMask;
        return (int) hash;
    }

    std::vector<SuggestItem> SymSpell::LookupCompound(const xstring &input) {
        return LookupCompound(input, this->maxDictionaryEditDistance);
    }

    std::vector<SuggestItem> SymSpell::LookupCompound(const xstring &input, int editDistanceMax) {
        std::vector<xstring> termList1 = ParseWords(input);

        std::vector<SuggestItem> suggestions;     //suggestions for a single term
        std::vector<SuggestItem> suggestionParts; //1 line with separate parts
        auto distanceComparer = EditDistance(this->distanceAlgorithm);

        bool lastCombi = false;
        for (int i = 0; i < termList1.size(); i++) {
            suggestions = Lookup(termList1[i], Top, editDistanceMax);

            if ((i > 0) && !lastCombi) {
                std::vector<SuggestItem> suggestionsCombi = Lookup(termList1[i - 1] + termList1[i], Top,
                                                                   editDistanceMax);

                if (!suggestionsCombi.empty()) {
                    SuggestItem best1 = suggestionParts[suggestionParts.size() - 1];
                    SuggestItem best2 = SuggestItem();
                    if (!suggestions.empty()) {
                        best2 = suggestions[0];
                    } else {
                        best2.term = termList1[i];
                        best2.distance = editDistanceMax + 1;
                        best2.count = (long) ((double) 10 / pow((double) 10, (double) best2.term.size())); // 0;
                    }

                    int distance1 = best1.distance + best2.distance;
                    if ((distance1 >= 0) && ((suggestionsCombi[0].distance + 1 < distance1) ||
                                             ((suggestionsCombi[0].distance + 1 == distance1) &&
                                              ((double) suggestionsCombi[0].count >
                                               (double) best1.count / (double) N * (double) best2.count)))) {
                        suggestionsCombi[0].distance++;
                        suggestionParts[suggestionParts.size() - 1] = suggestionsCombi[0];
                        lastCombi = true;
                        goto nextTerm;
                    }
                }
            }
            lastCombi = false;

            if ((!suggestions.empty()) && ((suggestions[0].distance == 0) || (termList1[i].size() == 1))) {
                suggestionParts.push_back(suggestions[0]);
            } else {
                SuggestItem suggestionSplitBest;

                if (!suggestions.empty()) suggestionSplitBest.set(suggestions[0]);

                if (termList1[i].size() > 1) {
                    for (int j = 1; j < termList1[i].size(); j++) {
                        xstring part1 = termList1[i].substr(0, j);
                        xstring part2 = termList1[i].substr(j);
                        SuggestItem suggestionSplit = SuggestItem();
                        std::vector<SuggestItem> suggestions1 = Lookup(part1, Top, editDistanceMax);
                        if (!suggestions1.empty()) {
                            std::vector<SuggestItem> suggestions2 = Lookup(part2, Top, editDistanceMax);
                            if (!suggestions2.empty()) {
                                suggestionSplit.term = suggestions1[0].term + XL(" ") + suggestions2[0].term;

                                int distance2 = distanceComparer.Compare(termList1[i], suggestionSplit.term,
                                                                         editDistanceMax);
                                if (distance2 < 0) distance2 = editDistanceMax + 1;

                                if (suggestionSplitBest.count) {
                                    if (distance2 > suggestionSplitBest.distance) continue;
                                    if (distance2 < suggestionSplitBest.distance) suggestionSplitBest.count = 0;
                                }

                                suggestionSplit.distance = distance2;
                                if (bigrams.count(suggestionSplit.term)) {
                                    long bigramCount = bigrams.at(suggestionSplit.term);
                                    suggestionSplit.count = bigramCount;
                                    if (!suggestions.empty()) {
                                        if ((suggestions1[0].term + suggestions2[0].term == termList1[i])) {
                                            suggestionSplit.count = fmax(suggestionSplit.count,
                                                                         suggestions[0].count + 2);
                                        } else if ((suggestions1[0].term == suggestions[0].term) ||
                                                   (suggestions2[0].term == suggestions[0].term)) {
                                            suggestionSplit.count = fmax(suggestionSplit.count,
                                                                         suggestions[0].count + 1);
                                        }
                                    } else if ((suggestions1[0].term + suggestions2[0].term == termList1[i])) {
                                        suggestionSplit.count = fmax(suggestionSplit.count,
                                                                     fmax(suggestions1[0].count,
                                                                          suggestions2[0].count) +
                                                                     2);
                                    }

                                } else {
                                    suggestionSplit.count = fmin(bigramCountMin,
                                                                 (int64_t) ((double) suggestions1[0].count /
                                                                            (double) N *
                                                                            (double) suggestions2[0].count));
                                }

                                if (suggestionSplit.count > suggestionSplitBest.count)
                                    suggestionSplitBest.set(suggestionSplit);
                            }
                        }
                    }

                    if (suggestionSplitBest.count) {
                        suggestionParts.push_back(suggestionSplitBest);
                    } else {
                        SuggestItem si = SuggestItem();
                        si.term = termList1[i];
                        si.count = (long) ((double) 10 / pow((double) 10, (double) si.term.size()));
                        si.distance = editDistanceMax + 1;
                        suggestionParts.push_back(si);
                    }
                } else {
                    SuggestItem si = SuggestItem();
                    si.term = termList1[i];
                    si.count = (long) ((double) 10 / pow((double) 10, (double) si.term.size()));
                    si.distance = editDistanceMax + 1;
                    suggestionParts.push_back(si);
                }
            }
            nextTerm:;
        }

        SuggestItem suggestion = SuggestItem();

        double count = N;
        xstring s;
        for (const SuggestItem &si : suggestionParts) {
            s += (si.term + XL(" "));
            count *= (double) si.count / (double) N;
        }
        suggestion.count = (long) count;
        rtrim(s);
        suggestion.term = s;
        suggestion.distance = distanceComparer.Compare(input, suggestion.term, MAXINT);

        std::vector<SuggestItem> suggestionsLine;
        suggestionsLine.push_back(suggestion);
        return suggestionsLine;
    }

    Info SymSpell::WordSegmentation(const xstring &input) {
        return WordSegmentation(input, this->MaxDictionaryEditDistance(), this->maxDictionaryWordLength);
    }

    Info SymSpell::WordSegmentation(const xstring &input, int maxEditDistance) {
        return WordSegmentation(input, maxEditDistance, this->maxDictionaryWordLength);
    }

    Info SymSpell::WordSegmentation(const xstring &input, int maxEditDistance, int maxSegmentationWordLength) {
        int arraySize = fmin(maxSegmentationWordLength, (int) input.size());
        std::vector<Info> compositions = std::vector<Info>(arraySize);
        int circularIndex = -1;

        for (int j = 0; j < input.size(); j++) {
            int imax = fmin((int) input.size() - j, maxSegmentationWordLength);
            for (int i = 1; i <= imax; i++) {
                xstring part = input.substr(j, i);
                int separatorLength = 0;
                int topEd = 0;
                double topProbabilityLog = 0;
                xstring topResult;

                if (isxspace(part[0])) {
                    part = part.substr(1);
                } else {
                    separatorLength = 1;
                }

                topEd += part.size();
                xregex r(XL("(\\s)+"));
                part = regex_replace(part, r, XL(""));
                topEd -= part.size();

                std::vector<SuggestItem> results = this->Lookup(part, Top, maxEditDistance);
                if (!results.empty()) {
                    topResult = results[0].term;
                    topEd += results[0].distance;
                    topProbabilityLog = log10((double) results[0].count / (double) N);
                } else {
                    topResult = part;
                    topEd += part.size();
                    topProbabilityLog = log10(10.0 / (N * pow(10.0, part.size())));
                }

                int destinationIndex = ((i + circularIndex) % arraySize);

                if (j == 0) {
                    compositions[destinationIndex].set(part, topResult, topEd, topProbabilityLog);
                } else if ((i == maxSegmentationWordLength)
                           || (((compositions[circularIndex].getDistance() + topEd ==
                                 compositions[destinationIndex].getDistance()) \
 || (compositions[circularIndex].getDistance() + separatorLength + topEd ==
     compositions[destinationIndex].getDistance())) \
 && (compositions[destinationIndex].getProbability() <
     compositions[circularIndex].getProbability() + topProbabilityLog))
                           || (compositions[circularIndex].getDistance() + separatorLength + topEd <
                               compositions[destinationIndex].getDistance())) {
                    xstring seg = compositions[circularIndex].getSegmented() + XL(" ") + part;
                    xstring correct = compositions[circularIndex].getCorrected() + XL(" ") + topResult;
                    int d = compositions[circularIndex].getDistance() + separatorLength + topEd;
                    double prob = compositions[circularIndex].getProbability() + topProbabilityLog;
                    compositions[destinationIndex].set(seg, correct, d, prob);
                }
            }
            circularIndex++;
            if (circularIndex == arraySize) circularIndex = 0;
        }
        return compositions[circularIndex];
    }

}