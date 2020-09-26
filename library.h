#pragma once
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <locale>
#include <regex>
#include <iostream>
#include "unordered_set"

#include "include/Helpers.h"
#include "include/EditDistance.h"

#define DEFAULT_SEPARATOR_CHAR XL('\t')
#define DEFAULT_MAX_EDIT_DISTANCE 2
#define DEFAULT_PREFIX_LENGTH 7
#define DEFAULT_COUNT_THRESHOLD 1
#define DEFAULT_INITIAL_CAPACITY 82765
#define DEFAULT_COMPACT_LEVEL 5
#define min3(a, b, c) (min(a, min(b, c)))
#define MAXINT LLONG_MAX
#define M
#define MAXLONG MAXINT
#define uint unsigned int
static inline void ltrim(std::string& s)
{

    s.erase(s.begin(),find_if(s.begin(), s.end(), [](char ch){
        return !isspace(ch);
    }));
}

static inline void rtrim(std::string& s)
{
    s.erase(find_if(s.rbegin(), s.rend(), [](char ch){
        return !isspace(ch);
    }).base(), s.end());
}

static inline void trim(std::string& s)
{
    ltrim(s);
    rtrim(s);
}

class Info
{
private:

    std::string segmentedstring;
    std::string correctedstring;
    int distanceSum;
    double probabilityLogSum;
public:
    void set(std::string& seg, std::string& cor, int d, double prob)
    {
        segmentedstring = seg;
        correctedstring = cor;
        distanceSum = d;
        probabilityLogSum = prob;
    };
    std::string getSegmented()
    {
        return segmentedstring;
    };
    std::string getCorrected()
    {
        return correctedstring;
    };
    int getDistance() const
    {
        return distanceSum;
    };
    double getProbability() const
    {
        return probabilityLogSum;
    };
};

enum Verbosity
{
    Top,
    Closest,
    All
};

class SymSpell
{
private:
    int initialCapacity;
    int maxDictionaryEditDistance;
    int prefixLength; //prefix length  5..7
    long countThreshold; //a threshold might be specified, when a term occurs so frequently in the corpus that it is considered a valid word for spelling correction
    int compactMask;
    DistanceAlgorithm distanceAlgorithm = DistanceAlgorithm::DamerauOSADistance;
    int maxDictionaryWordLength; //maximum std::unordered_map term length
    std::unordered_map<int, std::vector<std::string>> *deletes = nullptr;
    std::unordered_map<std::string, int64_t> words;
    std::unordered_map<std::string, int64_t> belowThresholdWords;

public:
    int MaxDictionaryEditDistance() const;

    int PrefixLength() const;

    int MaxLength() const;

    long CountThreshold() const;

    int WordCount();

    int EntryCount();

    explicit SymSpell(int initialCapacity = DEFAULT_INITIAL_CAPACITY, int maxDictionaryEditDistance = DEFAULT_MAX_EDIT_DISTANCE
            , int prefixLength = DEFAULT_PREFIX_LENGTH, int countThreshold = DEFAULT_COUNT_THRESHOLD
            , unsigned char compactLevel = DEFAULT_COMPACT_LEVEL);

    bool CreateDictionaryEntry(const std::string& key, int64_t count, SuggestionStage* staging);

    std::unordered_map<std::string, long> bigrams;
    int64_t bigramCountMin = MAXLONG;

    bool LoadBigramDictionary(const std::string& corpus, int termIndex, int countIndex, char separatorChars = DEFAULT_SEPARATOR_CHAR);

    bool LoadBigramDictionary(std::ifstream& corpusStream, int termIndex, int countIndex, char separatorChars = DEFAULT_SEPARATOR_CHAR);

    bool LoadDictionary(const std::string& corpus, int termIndex, int countIndex, char separatorChars = DEFAULT_SEPARATOR_CHAR);

    bool LoadDictionary(std::ifstream& corpusStream, int termIndex, int countIndex, char separatorChars = DEFAULT_SEPARATOR_CHAR);

    bool CreateDictionary(const std::string& corpus);

    bool CreateDictionary(std::ifstream& corpusStream);

    void PurgeBelowThresholdWords();

    void CommitStaged(SuggestionStage* staging);

    std::vector<SuggestItem> Lookup(std::string input, Verbosity verbosity);

    std::vector<SuggestItem> Lookup(std::string input, Verbosity verbosity, int maxEditDistance);

    std::vector<SuggestItem> Lookup(std::string input, Verbosity verbosity, int maxEditDistance, bool includeUnknown);

private:
    bool DeleteInSuggestionPrefix(std::string deleteSugg, int deleteLen, std::string suggestion, int suggestionLen) const;

    static std::vector<std::string> ParseWords(const std::string& text);

    std::unordered_set<std::string>* Edits(const std::string& word, int editDistance, std::unordered_set<std::string>* deleteWords);

    std::unordered_set<std::string> EditsPrefix(std::string key);

    int GetstringHash(std::string s) const;

public:
    std::vector<SuggestItem> LookupCompound(const std::string& input);

    std::vector<SuggestItem> LookupCompound(const std::string& input, int editDistanceMax);

    static const int64_t N = 1024908267229L;

    Info WordSegmentation(const std::string& input);

    Info WordSegmentation(const std::string& input, int maxEditDistance);

    Info WordSegmentation(const std::string& input, int maxEditDistance, int maxSegmentationWordLength);
};