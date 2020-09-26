#include "library.h"

#include <codecvt>

int SymSpell::MaxDictionaryEditDistance()
{
    return this->maxDictionaryEditDistance;
}

int SymSpell::PrefixLength()
{
    return this->prefixLength;
}

int SymSpell::MaxLength()
{
    return this->maxDictionaryWordLength;
}

long SymSpell::CountThreshold()
{
    return this->countThreshold;
}

int SymSpell::WordCount()
{
    return this->words.size();
}

int SymSpell::EntryCount()
{
    return this->deletes->size();
}

SymSpell::SymSpell(int initialCapacity, int maxDictionaryEditDistance
        , int prefixLength, int countThreshold, unsigned char compactLevel)
{
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
    this->words = Dictionary<xstring, int64_t>(initialCapacity);
}

bool SymSpell::CreateDictionaryEntry(xstring key, int64_t count, SuggestionStage* staging)
{

    if (count <= 0)
    {
        if (this->countThreshold > 0) return false; // no point doing anything if count is zero, as it can't change anything
        count = 0;
    }
    int countPrevious = -1;
    auto belowThresholdWordsFinded = belowThresholdWords.find(key);
    auto wordsFinded = words.find(key);
    if (countThreshold > 1 && belowThresholdWordsFinded != belowThresholdWords.end())
    {
        countPrevious = belowThresholdWordsFinded->second;
        count = (MAXINT - countPrevious > count) ? countPrevious + count : MAXINT;
        if (count >= countThreshold)
        {
            belowThresholdWords.erase(key);
        }
        else
        {
            belowThresholdWords.insert(pair<xstring, int64_t>(key, count));
            return false;
        }
    }
    else if (wordsFinded != words.end())
    {
        countPrevious = wordsFinded->second;
        count = (MAXINT - countPrevious > count) ? countPrevious + count : MAXINT;
        words.insert(pair<xstring, int64_t>(key, count));
        return false;
    }
    else if (count < CountThreshold())
    {
        belowThresholdWords.insert(pair<xstring, int64_t>(key, count));
        return false;
    }

    words.insert(pair<xstring, int64_t>(key, count));

    if (key.size() > this->maxDictionaryWordLength) this->maxDictionaryWordLength = key.size();

    //create deletes
    HashSet<xstring> edits = EditsPrefix(key);
    if (staging != NULL)
    {
        for (auto it = edits.begin(); it != edits.end(); ++it)
        {
            staging->Add(GetstringHash(*it), key);
        }
    }
    else
    {

        for (auto it = edits.begin(); it != edits.end(); ++it)
        {
            int deleteHash = GetstringHash(*it);
            auto deletesFinded = deletes->find(deleteHash);
            std::vector<xstring> suggestions;
            if(deletesFinded != deletes->end())
            {
                suggestions = deletesFinded->second;
                std::vector<xstring> newSuggestions(suggestions.size() + 1);
                for(int id=0; id<suggestions.size(); id++){
                    newSuggestions[id] = suggestions[id];
                }

                (*deletes)[deleteHash] = suggestions = newSuggestions;
            }
            else
            {
                suggestions = vector<xstring>(1);
                (*deletes).insert(pair<int, vector<xstring>>(deleteHash, suggestions));
            }
            suggestions[suggestions.size() - 1] = key;
        }

    }

    return true;
}

bool SymSpell::LoadBigramDictionary(string corpus, int termIndex, int countIndex, xchar separatorChars)
{
    xifstream corpusStream;
    corpusStream.open(corpus);
#ifdef UNICODE_SUPPORT
    locale utf8(locale(), new codecvt_utf8<wchar_t>);
	corpusStream.imbue(utf8);
#endif
    if (!corpusStream.is_open())
        return false;

    return LoadBigramDictionary(corpusStream, termIndex, countIndex, separatorChars);
}

bool SymSpell::LoadBigramDictionary(xifstream& corpusStream, int termIndex, int countIndex, xchar separatorChars)
{
    xstring line;
    int linePartsLength = (separatorChars == DEFAULT_SEPARATOR_CHAR) ? 3 : 2;
    while (getline(corpusStream, line))
    {
        vector<xstring> lineParts;
        xstringstream ss(line);
        xstring token;
        while (getline(ss, token, separatorChars))
            lineParts.push_back(token);
        xstring key;
        int64_t count;
        if (lineParts.size() >= linePartsLength)
        {
            key = (separatorChars == DEFAULT_SEPARATOR_CHAR) ? lineParts[termIndex] + XL(" ") + lineParts[termIndex + 1]: lineParts[termIndex];
            try{
                count = stoll(lineParts[countIndex]);

            }catch(...) {
                printf("Cannot convert %s to integer\n",lineParts[countIndex]);
            }
        }
        else
        {
            key = line;
            count = 1;
        }
        pair<xstring, int64_t> element(key, count);
        bigrams.insert(element);
        if (count < bigramCountMin) bigramCountMin = count;
    }

    if (bigrams.empty())
        return false;
    return true;
}

bool SymSpell::LoadDictionary(string corpus, int termIndex, int countIndex, xchar separatorChars)
{

    xifstream corpusStream(corpus);
#ifdef UNICODE_SUPPORT
    locale utf8(locale(), new codecvt_utf8<wchar_t>);
	corpusStream.imbue(utf8);
#endif
    if (!corpusStream.is_open())
        return false;

    return LoadDictionary(corpusStream, termIndex, countIndex, separatorChars);
}

bool SymSpell::LoadDictionary(xifstream& corpusStream, int termIndex, int countIndex, xchar separatorChars)
{
    SuggestionStage staging(16384);
    xstring line;
    int i = 0;
    int start, end;
    start = clock();
    while (getline(corpusStream, line))
    {
        i++;
        vector<xstring> lineParts;
        xstringstream ss(line);
        xstring token;
        while (getline(ss, token, separatorChars))
            lineParts.push_back(token);
        if (lineParts.size() >= 2)
        {
            int64_t count = stoll(lineParts[countIndex]);

            CreateDictionaryEntry(lineParts[termIndex], count, &staging);
        }
        else
        {
            CreateDictionaryEntry(line, 1, &staging);
        }

    }
    if (this->deletes == NULL)
        this->deletes = new Dictionary<int, vector<xstring>>(staging.DeleteCount());
    CommitStaged(&staging);
    if (this->EntryCount() == 0)
        return false;
    return true;
}

bool SymSpell::CreateDictionary(string corpus)
{
    xifstream corpusStream;
    corpusStream.open(corpus);
#ifdef UNICODE_SUPPORT
    locale utf8(locale(), new codecvt_utf8<wchar_t>);
	corpusStream.imbue(utf8);
#endif
    if (!corpusStream.is_open()) return false;

    return CreateDictionary(corpusStream);
}

/// <summary>Load multiple dictionary words from a stream containing plain text.</summary>
/// <remarks>Merges with any dictionary data already loaded.</remarks>
/// <param name="corpusStream">The stream containing the plain text.</param>
/// <returns>True if stream loads.</returns>
bool SymSpell::CreateDictionary(xifstream& corpusStream)
{
    xstring line;
    SuggestionStage staging = SuggestionStage(16384);
    while (getline(corpusStream, line))
    {
        for (xstring key : ParseWords(line))
        {
            CreateDictionaryEntry(key, 1, &staging);
        }

    }
    if (this->deletes == NULL) this->deletes = new Dictionary<int, vector<xstring>>(staging.DeleteCount());
    CommitStaged(&staging);
    if (this->EntryCount() == 0)
        return false;
    return true;
}

/// <summary>Remove all below threshold words from the dictionary.</summary>
/// <remarks>This can be used to reduce memory consumption after populating the dictionary from
/// a corpus using CreateDictionary.</remarks>
void SymSpell::PurgeBelowThresholdWords()
{
    belowThresholdWords.clear();
}

/// <summary>Commit staged dictionary additions.</summary>
/// <remarks>Used when you write your own process to load multiple words into the
/// dictionary, and as part of that process, you first created a SuggestionsStage
/// object, and passed that to CreateDictionaryEntry calls.</remarks>
/// <param name="staging">The SuggestionStage object storing the staged data.</param>
void SymSpell::CommitStaged(SuggestionStage* staging)
{
    staging->CommitTo(deletes);
}

/// <summary>Find suggested spellings for a given input word, using the maximum
/// edit distance specified during construction of the SymSpell dictionary.</summary>
/// <param name="input">The word being spell checked.</param>
/// <param name="verbosity">The value controlling the quantity/closeness of the retuned suggestions.</param>
/// <returns>A vector of SuggestItem object representing suggested correct spellings for the input word,
/// sorted by edit distance, and secondarily by count frequency.</returns>
vector<SuggestItem> SymSpell::Lookup(xstring input, Verbosity verbosity)
{
    return Lookup(input, verbosity, this->maxDictionaryEditDistance, false);
}

/// <summary>Find suggested spellings for a given input word, using the maximum
/// edit distance specified during construction of the SymSpell dictionary.</summary>
/// <param name="input">The word being spell checked.</param>
/// <param name="verbosity">The value controlling the quantity/closeness of the retuned suggestions.</param>
/// <param name="maxEditDistance">The maximum edit distance between input and suggested words.</param>
/// <returns>A vector of SuggestItem object representing suggested correct spellings for the input word,
/// sorted by edit distance, and secondarily by count frequency.</returns>
vector<SuggestItem> SymSpell::Lookup(xstring input, Verbosity verbosity, int maxEditDistance)
{
    return Lookup(input, verbosity, maxEditDistance, false);
}

/// <summary>Find suggested spellings for a given input word.</summary>
/// <param name="input">The word being spell checked.</param>
/// <param name="verbosity">The value controlling the quantity/closeness of the retuned suggestions.</param>
/// <param name="maxEditDistance">The maximum edit distance between input and suggested words.</param>
/// <param name="includeUnknown">Include input word in suggestions, if no words within edit distance found.</param>
/// <returns>A vector of SuggestItem object representing suggested correct spellings for the input word,
/// sorted by edit distance, and secondarily by count frequency.</returns>
vector<SuggestItem> SymSpell::Lookup(xstring input, Verbosity verbosity, int maxEditDistance, bool includeUnknown)
{
    //verbosity=Top: the suggestion with the highest term frequency of the suggestions of smallest edit distance found
    //verbosity=Closest: all suggestions of smallest edit distance found, the suggestions are ordered by term frequency
    //verbosity=All: all suggestions <= maxEditDistance, the suggestions are ordered by edit distance, then by term frequency (slower, no early termination)

    // maxEditDistance used in Lookup can't be bigger than the maxDictionaryEditDistance
    // used to construct the underlying dictionary structure.
    int skip = 0;
    if (maxEditDistance > this->maxDictionaryEditDistance) throw std::invalid_argument("maxEditDistance");

    vector<SuggestItem> suggestions;
    int inputLen = input.size();
    // early exit - word is too big to possibly match any words
    if (inputLen - maxEditDistance > this->maxDictionaryWordLength) skip = 1;

    // quick look for exact match
    int64_t suggestionCount = 0;
    if (words.count(input) && !skip)
    {
        suggestionCount = words.at(input);
        suggestions.push_back(SuggestItem(input, 0, suggestionCount));
        // early exit - return exact match, unless caller wants all matches
        if (verbosity != All) skip = 1;
    }

    //early termination, if we only want to check if word in dictionary or get its frequency e.g. for word segmentation
    if (maxEditDistance == 0) skip = 1;

    if (!skip)
    {
        // deletes we've considered already
        HashSet<xstring> hashset1;
        // suggestions we've considered already
        HashSet<xstring> hashset2;
        // we considered the input already in the word.TryGetValue above
        hashset2.insert(input);

        int maxEditDistance2 = maxEditDistance;
        int candidatePointer = 0;
        vector<xstring> singleSuggestion = { XL("") };
        vector<xstring> candidates;

        //add original prefix
        int inputPrefixLen = inputLen;
        if (inputPrefixLen > prefixLength)
        {
            inputPrefixLen = prefixLength;
            candidates.push_back(input.substr(0, inputPrefixLen));
        }
        else
        {
            candidates.push_back(input);
        }
        auto distanceComparer = EditDistance(this->distanceAlgorithm);
        while (candidatePointer < candidates.size())
        {
            xstring candidate = candidates[candidatePointer++];
            int candidateLen = candidate.size();
            int lengthDiff = inputPrefixLen - candidateLen;

            //save some time - early termination
            //if candidate distance is already higher than suggestion distance, than there are no better suggestions to be expected
            if (lengthDiff > maxEditDistance2)
            {
                // skip to next candidate if Verbosity.All, look no further if Verbosity.Top or Closest
                // (candidates are ordered by delete distance, so none are closer than current)
                if (verbosity == Verbosity::All) continue;
                break;
            }

            //read candidate entry from dictionary
            if (deletes->count(GetstringHash(candidate)))
            {
                vector<xstring> dictSuggestions = deletes->at(GetstringHash(candidate));
                //iterate through suggestions (to other correct dictionary items) of delete item and add them to suggestion list
                for (int i = 0; i < dictSuggestions.size(); i++)
                {
                    auto suggestion = dictSuggestions[i];
                    int suggestionLen = suggestion.size();
                    if (suggestion == input) continue;
                    if ((abs(suggestionLen - inputLen) > maxEditDistance2) // input and sugg lengths diff > allowed/current best distance
                        || (suggestionLen < candidateLen) // sugg must be for a different delete string, in same bin only because of hash collision
                        || (suggestionLen == candidateLen && suggestion != candidate)) // if sugg len = delete len, then it either equals delete or is in same bin only because of hash collision
                        continue;
                    auto suggPrefixLen = min(suggestionLen, prefixLength);
                    if (suggPrefixLen > inputPrefixLen && (suggPrefixLen - candidateLen) > maxEditDistance2) continue;

                    //True Damerau-Levenshtein Edit Distance: adjust distance, if both distances>0
                    //We allow simultaneous edits (deletes) of maxEditDistance on on both the dictionary and the input term.
                    //For replaces and adjacent transposes the resulting edit distance stays <= maxEditDistance.
                    //For inserts and deletes the resulting edit distance might exceed maxEditDistance.
                    //To prevent suggestions of a higher edit distance, we need to calculate the resulting edit distance, if there are simultaneous edits on both sides.
                    //Example: (bank==bnak and bank==bink, but bank!=kanb and bank!=xban and bank!=baxn for maxEditDistance=1)
                    //Two deletes on each side of a pair makes them all equal, but the first two pairs have edit distance=1, the others edit distance=2.
                    int distance = 0;
                    int min_len = 0;
                    if (candidateLen == 0)
                    {
                        //suggestions which have no common chars with input (inputLen<=maxEditDistance && suggestionLen<=maxEditDistance)
                        distance = max(inputLen, suggestionLen);
                        auto flag = hashset2.insert(suggestion);
                        if (distance > maxEditDistance2 || !flag.second) continue;
                    }
                    else if (suggestionLen == 1)
                    {
                        // not entirely sure what happens here yet
                        if (input.find(suggestion[0]) == input.npos)
                            distance = inputLen;
                        else
                            distance = inputLen - 1;

                        auto flag = hashset2.insert(suggestion);
                        if (distance > maxEditDistance2 || !flag.second) continue;
                    }
                    else
                        //number of edits in prefix ==maxediddistance  AND no identic suffix
                        //, then editdistance>maxEditDistance and no need for Levenshtein calculation
                        //      (inputLen >= prefixLength) && (suggestionLen >= prefixLength)
                    if ((prefixLength - maxEditDistance == candidateLen)
                        && (((min_len = min(inputLen, suggestionLen) - prefixLength) > 1)
                            && (input.substr(inputLen + 1 - min_len) != suggestion.substr(suggestionLen + 1 - min_len)))
                        || ((min_len > 0) && (input[inputLen - min_len] != suggestion[suggestionLen - min_len])
                            && ((input[inputLen - min_len - 1] != suggestion[suggestionLen - min_len])
                                || (input[inputLen - min_len] != suggestion[suggestionLen - min_len - 1]))))
                    {
                        continue;
                    }
                    else
                    {
                        // DeleteInSuggestionPrefix is somewhat expensive, and only pays off when verbosity is Top or Closest.
                        if ((verbosity != All && !DeleteInSuggestionPrefix(candidate, candidateLen, suggestion, suggestionLen))
                            || !hashset2.insert(suggestion).second) continue;
                        distance = distanceComparer.Compare(input, suggestion, maxEditDistance2);
                        if (distance < 0) continue;
                    }

                    //save some time
                    //do not process higher distances than those already found, if verbosity<All (note: maxEditDistance2 will always equal maxEditDistance when Verbosity.All)
                    if (distance <= maxEditDistance2)
                    {
                        suggestionCount = words[suggestion];
                        SuggestItem si = SuggestItem(suggestion, distance, suggestionCount);
                        if (suggestions.size() > 0)
                        {
                            switch (verbosity)
                            {
                                case Closest:
                                {
                                    //we will calculate DamLev distance only to the smallest found distance so far
                                    if (distance < maxEditDistance2) suggestions.clear();
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
                            }
                        }
                        if (verbosity != All) maxEditDistance2 = distance;
                        suggestions.push_back(si);
                    }
                }//end foreach
            }//end if

            //add edits
            //derive edits (deletes) from candidate (input) and add them to candidates list
            //this is a recursive process until the maximum edit distance has been reached
            if ((lengthDiff < maxEditDistance) && (candidateLen <= prefixLength))
            {
                //save some time
                //do not create edits with edit distance smaller than suggestions already found
                if (verbosity != All && lengthDiff >= maxEditDistance2) continue;

                for (int i = 0; i < candidateLen; i++)
                {
                    xstring temp(candidate);
                    xstring del = temp.erase(i, 1);

                    if (hashset1.insert(del).second) { candidates.push_back(del); }
                }
            }
        }//end while

        //sort by ascending edit distance, then by descending word frequency
        if (suggestions.size() > 1) sort(suggestions.begin(), suggestions.end(), [](SuggestItem& l, SuggestItem& r){
                return l.CompareTo(r) < 0? 1 : 0;
            });
    }
    if (includeUnknown && (suggestions.size() == 0)) suggestions.push_back(SuggestItem(input, maxEditDistance + 1, 0));
    return suggestions;
}//end if

//check whether all delete chars are present in the suggestion prefix in correct order, otherwise this is just a hash collision
bool SymSpell::DeleteInSuggestionPrefix(xstring deleteSugg, int deleteLen, xstring suggestion, int suggestionLen)
{
    if (deleteLen == 0) return true;
    if (prefixLength < suggestionLen) suggestionLen = prefixLength;
    int j = 0;
    for (int i = 0; i < deleteLen; i++)
    {
        xchar delChar = deleteSugg[i];
        while (j < suggestionLen && delChar != suggestion[j]) j++;
        if (j == suggestionLen) return false;
    }
    return true;
}

//create a non-unique wordlist from sample text
//language independent (e.g. works with Chinese characters)
vector<xstring> SymSpell::ParseWords(xstring text)
{
    // \w Alphanumeric characters (including non-latin characters, umlaut characters and digits) plus "_"
    // \d Digits
    // Compatible with non-latin characters, does not split words at apostrophes
    xregex r(XL("['’\\w-\\[_\\]]+"));
    xsmatch m;
    vector<xstring> matches;
    //for benchmarking only: with CreateDictionary("big.txt","") and the text corpus from http://norvig.com/big.txt  the Regex below provides the exact same number of dictionary items as Norvigs regex "[a-z]+" (which splits words at apostrophes & incompatible with non-latin characters)
    //MatchCollection mc = Regex.Matches(text.ToLower(), @"[\w-[\d_]]+");
    xstring::const_iterator ptr(text.cbegin());
    while(regex_search(ptr, text.cend(),m,r))
    {
        matches.push_back(m[0]);
        ptr = m.suffix().first;
    }
    return matches;
}

//inexpensive and language independent: only deletes, no transposes + replaces + inserts
//replaces and inserts are expensive and language dependent (Chinese has 70,000 Unicode Han characters)
HashSet<xstring>* SymSpell::Edits(xstring word, int editDistance, HashSet<xstring>* deleteWords)
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
                //recursion, if maximum edit distance not yet reached
                if (editDistance < maxDictionaryEditDistance) Edits(del, editDistance, deleteWords);
            }
        }
    }
    return deleteWords;
}

HashSet<xstring> SymSpell::EditsPrefix(xstring key)
{
    HashSet<xstring> hashSet = HashSet<xstring>();
    if (key.size() <= maxDictionaryEditDistance) hashSet.insert(XL(""));
    if (key.size() > prefixLength) key = key.substr(0, prefixLength);
    hashSet.insert(key);
    Edits(key, 0, &hashSet);
    return hashSet;
}

int SymSpell::GetstringHash(xstring s)
{
    //return s.GetHashCode();

    int len = s.size();
    int lenMask = len;
    if (lenMask > 3) lenMask = 3;

    uint hash = 2166136261;
    for (auto i = 0; i < len; i++)
    {
        //unchecked, its fine even if it can be overflowed
        {
            hash ^= s[i];
            hash *= 16777619;
        }
    }

    hash &= this->compactMask;
    hash |= (uint)lenMask;
    return (int)hash;
}


//######################

//LookupCompound supports compound aware automatic spelling correction of multi-word input strings with three cases:
//1. mistakenly inserted space into a correct word led to two incorrect terms
//2. mistakenly omitted space between two correct words led to one incorrect combined term
//3. multiple independent input terms with/without spelling errors

/// <summary>Find suggested spellings for a multi-word input string (supports word splitting/merging).</summary>
/// <param name="input">The string being spell checked.</param>
/// <returns>A vector of SuggestItem object representing suggested correct spellings for the input string.</returns>
vector<SuggestItem> SymSpell::LookupCompound(xstring input)
{
    return LookupCompound(input, this->maxDictionaryEditDistance);
}

/// <summary>Find suggested spellings for a multi-word input string (supports word splitting/merging).</summary>
/// <param name="input">The string being spell checked.</param>
/// <param name="maxEditDistance">The maximum edit distance between input and suggested words.</param>
/// <returns>A vector of SuggestItem object representing suggested correct spellings for the input string.</returns>
vector<SuggestItem> SymSpell::LookupCompound(xstring input, int editDistanceMax)
{
    //parse input string into single terms
    vector<xstring> termList1 = ParseWords(input);

    vector<SuggestItem> suggestions;     //suggestions for a single term
    vector<SuggestItem> suggestionParts; //1 line with separate parts
    auto distanceComparer = EditDistance(this->distanceAlgorithm);

    //translate every term to its best suggestion, otherwise it remains unchanged
    bool lastCombi = false;
    for (int i = 0; i < termList1.size(); i++)
    {
        suggestions = Lookup(termList1[i], Top, editDistanceMax);

        //combi check, always before split
        if ((i > 0) && !lastCombi)
        {
            vector<SuggestItem> suggestionsCombi = Lookup(termList1[i - 1] + termList1[i], Top, editDistanceMax);

            if (suggestionsCombi.size() > 0)
            {
                SuggestItem best1 = suggestionParts[suggestionParts.size() - 1];
                SuggestItem best2 = SuggestItem();
                if (suggestions.size() > 0)
                {
                    best2 = suggestions[0];
                }
                else
                {
                    //unknown word
                    best2.term = termList1[i];
                    //estimated edit distance
                    best2.distance = editDistanceMax + 1;
                    //estimated word occurrence probability P=10 / (N * 10^word length l)
                    best2.count = (long)((double)10 / pow((double)10, (double)best2.term.size())); // 0;
                }

                //distance1=edit distance between 2 split terms und their best corrections : als comparative value for the combination
                int distance1 = best1.distance + best2.distance;
                if ((distance1 >= 0) && ((suggestionsCombi[0].distance + 1 < distance1) || ((suggestionsCombi[0].distance + 1 == distance1) && ((double)suggestionsCombi[0].count > (double)best1.count / (double)N * (double)best2.count))))
                {
                    suggestionsCombi[0].distance++;
                    suggestionParts[suggestionParts.size() - 1] = suggestionsCombi[0];
                    lastCombi = true;
                    goto nextTerm;
                }
            }
        }
        lastCombi = false;

        //alway split terms without suggestion / never split terms with suggestion ed=0 / never split single char terms
        if ((suggestions.size() > 0) && ((suggestions[0].distance == 0) || (termList1[i].size() == 1)))
        {
            //choose best suggestion
            suggestionParts.push_back(suggestions[0]);
        }
        else
        {
            //if no perfect suggestion, split word into pairs
            SuggestItem suggestionSplitBest;

            //add original term
            if (suggestions.size() > 0) suggestionSplitBest.set(suggestions[0]);

            if (termList1[i].size() > 1)
            {
                for (int j = 1; j < termList1[i].size(); j++)
                {
                    xstring part1 = termList1[i].substr(0, j);
                    xstring part2 = termList1[i].substr(j);
                    SuggestItem suggestionSplit = SuggestItem();
                    vector<SuggestItem> suggestions1 = Lookup(part1, Top, editDistanceMax);
                    if (suggestions1.size() > 0)
                    {
                        vector<SuggestItem> suggestions2 = Lookup(part2, Top, editDistanceMax);
                        if (suggestions2.size() > 0)
                        {
                            //select best suggestion for split pair
                            suggestionSplit.term = suggestions1[0].term + XL(" ") + suggestions2[0].term;

                            int distance2 = distanceComparer.Compare(termList1[i], suggestionSplit.term, editDistanceMax);
                            if (distance2 < 0) distance2 = editDistanceMax + 1;

                            if (suggestionSplitBest.count)
                            {
                                if (distance2 > suggestionSplitBest.distance) continue;
                                if (distance2 < suggestionSplitBest.distance) suggestionSplitBest.count = 0;
                            }

                            suggestionSplit.distance = distance2;
                            //if bigram exists in bigram dictionary
                            if (bigrams.count(suggestionSplit.term))
                            {
                                long bigramCount = bigrams.at(suggestionSplit.term);
                                suggestionSplit.count = bigramCount;

                                //increase count, if split.corrections are part of or identical to input
                                //single term correction exists
                                if (suggestions.size() > 0)
                                {
                                    //alternatively remove the single term from suggestionsSplit, but then other splittings could win
                                    if ((suggestions1[0].term + suggestions2[0].term == termList1[i]))
                                    {
                                        //make count bigger than count of single term correction
                                        suggestionSplit.count = max(suggestionSplit.count, suggestions[0].count + 2);
                                    }
                                    else if ((suggestions1[0].term == suggestions[0].term) || (suggestions2[0].term == suggestions[0].term))
                                    {
                                        //make count bigger than count of single term correction
                                        suggestionSplit.count = max(suggestionSplit.count, suggestions[0].count + 1);
                                    }
                                }
                                    //no single term correction exists
                                else if ((suggestions1[0].term + suggestions2[0].term == termList1[i]))
                                {
                                    suggestionSplit.count = max(suggestionSplit.count, max(suggestions1[0].count, suggestions2[0].count) + 2);
                                }

                            }
                            else
                            {
                                //The Naive Bayes probability of the word combination is the product of the two word probabilities: P(AB) = P(A) * P(B)
                                //use it to estimate the frequency count of the combination, which then is used to rank/select the best splitting variant
                                suggestionSplit.count = min(bigramCountMin, (int64_t)((double)suggestions1[0].count / (double)N * (double)suggestions2[0].count));
                            }

                            if (suggestionSplit.count > suggestionSplitBest.count) suggestionSplitBest.set(suggestionSplit);
                        }
                    }
                }

                if (suggestionSplitBest.count)
                {
                    //select best suggestion for split pair
                    suggestionParts.push_back(suggestionSplitBest);
                }
                else
                {
                    SuggestItem si = SuggestItem();
                    si.term = termList1[i];
                    //estimated word occurrence probability P=10 / (N * 10^word length l)
                    si.count = (long)((double)10 / pow((double)10, (double)si.term.size()));
                    si.distance = editDistanceMax + 1;
                    suggestionParts.push_back(si);
                }
            }
            else
            {
                SuggestItem si = SuggestItem();
                si.term = termList1[i];
                //estimated word occurrence probability P=10 / (N * 10^word length l)
                si.count = (long)((double)10 / pow((double)10, (double)si.term.size()));
                si.distance = editDistanceMax + 1;
                suggestionParts.push_back(si);
            }
        }
        nextTerm:;
    }

    SuggestItem suggestion = SuggestItem();

    double count = N;
    xstring s(XL("")) ;
    for (SuggestItem si : suggestionParts)
    {
        s += (si.term + XL(" "));
        count *= (double)si.count / (double)N;
    }
    suggestion.count = (long)count;
    rtrim(s);
    suggestion.term = s;
    suggestion.distance = distanceComparer.Compare(input, suggestion.term, MAXINT);

    vector<SuggestItem> suggestionsLine;
    suggestionsLine.push_back(suggestion);
    return suggestionsLine;
}

/// <summary>Find suggested spellings for a multi-word input string (supports word splitting/merging).</summary>
/// <param name="input">The string being spell checked.</param>
/// <returns>The word segmented string,
/// the word segmented and spelling corrected string,
/// the Edit distance sum between input string and corrected string,
/// the Sum of word occurence probabilities in log scale (a measure of how common and probable the corrected segmentation is).</returns>
Info SymSpell::WordSegmentation(xstring input)
{
    return WordSegmentation(input, this->MaxDictionaryEditDistance(), this->maxDictionaryWordLength);
}

/// <summary>Find suggested spellings for a multi-word input string (supports word splitting/merging).</summary>
/// <param name="input">The string being spell checked.</param>
/// <param name="maxEditDistance">The maximum edit distance between input and corrected words
/// (0=no correction/segmentation only).</param>
/// <returns>The word segmented string,
/// the word segmented and spelling corrected string,
/// the Edit distance sum between input string and corrected string,
/// the Sum of word occurence probabilities in log scale (a measure of how common and probable the corrected segmentation is).</returns>
Info SymSpell::WordSegmentation(xstring input, int maxEditDistance)
{
    return WordSegmentation(input, maxEditDistance, this->maxDictionaryWordLength);
}

/// <summary>Find suggested spellings for a multi-word input string (supports word splitting/merging).</summary>
/// <param name="input">The string being spell checked.</param>
/// <param name="maxSegmentationWordLength">The maximum word length that should be considered.</param>
/// <param name="maxEditDistance">The maximum edit distance between input and corrected words
/// (0=no correction/segmentation only).</param>
/// <returns>The word segmented string,
/// the word segmented and spelling corrected string,
/// the Edit distance sum between input string and corrected string,
/// the Sum of word occurence probabilities in log scale (a measure of how common and probable the corrected segmentation is).</returns>
Info SymSpell::WordSegmentation(xstring input, int maxEditDistance, int maxSegmentationWordLength)
{
    int arraySize = min(maxSegmentationWordLength, (int)input.size());
    vector<Info> compositions = vector<Info>(arraySize);
    int circularIndex = -1;

    //outer loop (column): all possible part start positions
    for (int j = 0; j < input.size(); j++)
    {
        //inner loop (row): all possible part lengths (from start position): part can't be bigger than longest word in dictionary (other than long unknown word)
        int imax = min((int)input.size() - j, maxSegmentationWordLength);
        for (int i = 1; i <= imax; i++)
        {
            //get top spelling correction/ed for part
            xstring part = input.substr(j, i);
            int separatorLength = 0;
            int topEd = 0;
            double topProbabilityLog = 0;
            xstring topResult = XL("");

            if (isxspace(part[0]))
            {
                //remove space for levensthein calculation
                part = part.substr(1);
            }
            else
            {
                //add ed+1: space did not exist, had to be inserted
                separatorLength = 1;
            }

            //remove space from part1, add number of removed spaces to topEd
            topEd += part.size();
            //remove space
            xregex r(XL("(\\s)+"));
            part = regex_replace(part, r, XL(""));
            // part = part.Replace(" ", ""); //=System.Text.RegularExpressions.Regex.Replace(part1, @"\s+", "");
            // 							  //add number of removed spaces to ed
            topEd -= part.size();

            vector<SuggestItem> results = this->Lookup(part, Top, maxEditDistance);
            if (results.size() > 0)
            {
                topResult = results[0].term;
                topEd += results[0].distance;
                //Naive Bayes Rule
                //we assume the word probabilities of two words to be independent
                //therefore the resulting probability of the word combination is the product of the two word probabilities

                //instead of computing the product of probabilities we are computing the sum of the logarithm of probabilities
                //because the probabilities of words are about 10^-10, the product of many such small numbers could exceed (underflow) the floating number range and become zero
                //log(ab)=log(a)+log(b)
                topProbabilityLog = log10((double)results[0].count / (double)N);
            }
            else
            {
                topResult = part;
                //default, if word not found
                //otherwise long input text would win as long unknown word (with ed=edmax+1 ), although there there should many spaces inserted
                topEd += part.size();
                topProbabilityLog = log10(10.0 / (N * pow(10.0, part.size())));
            }

            int destinationIndex = ((i + circularIndex) % arraySize);

            //set values in first loop
            if (j == 0)
            {
                compositions[destinationIndex].set(part, topResult, topEd, topProbabilityLog);
            }
            else if ((i == maxSegmentationWordLength)
                     //replace values if better probabilityLogSum, if same edit distance OR one space difference
                     || (((compositions[circularIndex].getDistance() + topEd == compositions[destinationIndex].getDistance()) \
					|| (compositions[circularIndex].getDistance() + separatorLength + topEd == compositions[destinationIndex].getDistance())) \
					&& (compositions[destinationIndex].getProbability() < compositions[circularIndex].getProbability() + topProbabilityLog))
                     //replace values if smaller edit distance
                     || (compositions[circularIndex].getDistance() + separatorLength + topEd < compositions[destinationIndex].getDistance()))
            {
                xstring seg = compositions[circularIndex].getSegmented()+ XL(" ") + part;
                xstring correct = compositions[circularIndex].getCorrected() + XL(" ") + topResult;
                int d = compositions[circularIndex].getDistance() + separatorLength + topEd;
                double prob = compositions[circularIndex].getProbability() + topProbabilityLog;
                compositions[destinationIndex].set(seg, correct, d, prob);
            }
        }
        circularIndex++; if (circularIndex == arraySize) circularIndex = 0;
    }
    return compositions[circularIndex];
}