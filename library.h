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

/// <summary>Controls the closeness/quantity of returned spelling suggestions.</summary>
enum Verbosity
{
    /// <summary>Top suggestion with the highest term frequency of the suggestions of smallest edit distance found.</summary>
    Top,
    /// <summary>All suggestions of smallest edit distance found, suggestions ordered by term frequency.</summary>
    Closest,
    /// <summary>All suggestions within maxEditDistance, suggestions ordered by edit distance
    /// , then by term frequency (slower, no early termination).</summary>
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
    // std::unordered_map that contains a mapping of lists of suggested correction words to the hashCodes
    // of the original words and the deletes derived from them. Collisions of hashCodes is tolerated,
    // because suggestions are ultimately verified via an edit distance function.
    // A list of suggestions might have a single suggestion, or multiple suggestions.
    std::unordered_map<int, std::vector<std::string>> *deletes = nullptr;
    // std::unordered_map of unique correct spelling words, and the frequency count for each word.
    std::unordered_map<std::string, int64_t> words;
    // std::unordered_map of unique words that are below the count threshold for being considered correct spellings.
    std::unordered_map<std::string, int64_t> belowThresholdWords;

public:
    /// <summary>Maximum edit distance for std::unordered_map precalculation.</summary>
    int MaxDictionaryEditDistance();

    /// <summary>Length of prefix, from which deletes are generated.</summary>
    int PrefixLength();

    /// <summary>Length of longest word in the std::unordered_map.</summary>
    int MaxLength();

    /// <summary>Count threshold for a word to be considered a valid word for spelling correction.</summary>
    long CountThreshold();

    /// <summary>Number of unique words in the std::unordered_map.</summary>
    int WordCount();

    /// <summary>Number of word prefixes and intermediate word deletes encoded in the std::unordered_map.</summary>
    int EntryCount();

    /// <summary>Create a new instanc of SymSpell.</summary>
    /// <remarks>Specifying ann accurate initialCapacity is not essential,
    /// but it can help speed up processing by alleviating the need for
    /// data restructuring as the size grows.</remarks>
    /// <param name="initialCapacity">The expected number of words in std::unordered_map.</param>
    /// <param name="maxDictionaryEditDistance">Maximum edit distance for doing lookups.</param>
    /// <param name="prefixLength">The length of word prefixes used for spell checking..</param>
    /// <param name="countThreshold">The minimum frequency count for std::unordered_map words to be considered correct spellings.</param>
    /// <param name="compactLevel">Degree of favoring lower memory use over speed (0=fastest,most memory, 16=slowest,least memory).</param>

    explicit SymSpell(int initialCapacity = DEFAULT_INITIAL_CAPACITY, int maxDictionaryEditDistance = DEFAULT_MAX_EDIT_DISTANCE
            , int prefixLength = DEFAULT_PREFIX_LENGTH, int countThreshold = DEFAULT_COUNT_THRESHOLD
            , unsigned char compactLevel = DEFAULT_COMPACT_LEVEL);

    /// <summary>Create/Update an entry in the std::unordered_map.</summary>
    /// <remarks>For every word there are deletes with an edit distance of 1..maxEditDistance created and added to the
    /// std::unordered_map. Every delete entry has a suggestions list, which points to the original term(s) it was created from.
    /// The std::unordered_map may be dynamically updated (word frequency and new words) at any time by calling CreateDictionaryEntry</remarks>
    /// <param name="key">The word to add to std::unordered_map.</param>
    /// <param name="count">The frequency count for word.</param>
    /// <param name="staging">Optional staging object to speed up adding many entries by staging them to a temporary structure.</param>
    /// <returns>True if the word was added as a new correctly spelled word,
    /// or false if the word is added as a below threshold word, or updates an
    /// existing correctly spelled word.</returns>
    bool CreateDictionaryEntry(std::string key, int64_t count, SuggestionStage* staging);

    std::unordered_map<std::string, long> bigrams;
    int64_t bigramCountMin = MAXLONG;

    /// <summary>Load multiple std::unordered_map entries from a file of word/frequency count pairs</summary>
    /// <remarks>Merges with any std::unordered_map data already loaded.</remarks>
    /// <param name="corpus">The path+filename of the file.</param>
    /// <param name="termIndex">The column position of the word.</param>
    /// <param name="countIndex">The column position of the frequency count.</param>
    /// <param name="separatorChars">Separator characters between term(s) and count.</param>
    /// <returns>True if file loaded, or false if file not found.</returns>
    bool LoadBigramDictionary(std::string corpus, int termIndex, int countIndex, char separatorChars = DEFAULT_SEPARATOR_CHAR);

    /// <summary>Load multiple std::unordered_map entries from a file of word/frequency count pairs</summary>
    /// <remarks>Merges with any std::unordered_map data already loaded.</remarks>
    /// <param name="corpus">The path+filename of the file.</param>
    /// <param name="termIndex">The column position of the word.</param>
    /// <param name="countIndex">The column position of the frequency count.</param>
    /// <param name="separatorChars">Separator characters between term(s) and count.</param>
    /// <returns>True if file loaded, or false if file not found.</returns>
    bool LoadBigramDictionary(std::ifstream& corpusStream, int termIndex, int countIndex, char separatorChars = DEFAULT_SEPARATOR_CHAR);

    /// <summary>Load multiple std::unordered_map entries from a file of word/frequency count pairs</summary>
    /// <remarks>Merges with any std::unordered_map data already loaded.</remarks>
    /// <param name="corpus">The path+filename of the file.</param>
    /// <param name="termIndex">The column position of the word.</param>
    /// <param name="countIndex">The column position of the frequency count.</param>
    /// <param name="separatorChars">Separator characters between term(s) and count.</param>
    /// <returns>True if file loaded, or false if file not found.</returns>
    bool LoadDictionary(std::string corpus, int termIndex, int countIndex, char separatorChars = DEFAULT_SEPARATOR_CHAR);

    /// <summary>Load multiple std::unordered_map entries from a stream of word/frequency count pairs</summary>
    /// <remarks>Merges with any std::unordered_map data already loaded.</remarks>
    /// <param name="corpusStream">The stream containing the word/frequency count pairs.</param>
    /// <param name="termIndex">The column position of the word.</param>
    /// <param name="countIndex">The column position of the frequency count.</param>
    /// <param name="separatorChars">Separator characters between term(s) and count.</param>
    /// <returns>True if stream loads.</returns>
    bool LoadDictionary(std::ifstream& corpusStream, int termIndex, int countIndex, char separatorChars = DEFAULT_SEPARATOR_CHAR);

    /// <summary>Load multiple std::unordered_map words from a file containing plain text.</summary>
    /// <remarks>Merges with any std::unordered_map data already loaded.</remarks>
    /// <param name="corpus">The path+filename of the file.</param>
    /// <returns>True if file loaded, or false if file not found.</returns>
    bool CreateDictionary(std::string corpus);

    /// <summary>Load multiple std::unordered_map words from a stream containing plain text.</summary>
    /// <remarks>Merges with any std::unordered_map data already loaded.</remarks>
    /// <param name="corpusStream">The stream containing the plain text.</param>
    /// <returns>True if stream loads.</returns>
    bool CreateDictionary(std::ifstream& corpusStream);

    /// <summary>Remove all below threshold words from the std::unordered_map.</summary>
    /// <remarks>This can be used to reduce memory consumption after populating the std::unordered_map from
    /// a corpus using CreateDictionary.</remarks>
    void PurgeBelowThresholdWords();

    /// <summary>Commit staged std::unordered_map additions.</summary>
    /// <remarks>Used when you write your own process to load multiple words into the
    /// std::unordered_map, and as part of that process, you first created a SuggestionsStage
    /// object, and passed that to CreateDictionaryEntry calls.</remarks>
    /// <param name="staging">The SuggestionStage object storing the staged data.</param>
    void CommitStaged(SuggestionStage* staging);

    /// <summary>Find suggested spellings for a given input word, using the maximum
    /// edit distance specified during construction of the SymSpell std::unordered_map.</summary>
    /// <param name="input">The word being spell checked.</param>
    /// <param name="verbosity">The value controlling the quantity/closeness of the retuned suggestions.</param>
    /// <returns>A List of SuggestItem object representing suggested correct spellings for the input word,
    /// sorted by edit distance, and secondarily by count frequency.</returns>
    std::vector<SuggestItem> Lookup(std::string input, Verbosity verbosity);

    /// <summary>Find suggested spellings for a given input word, using the maximum
    /// edit distance specified during construction of the SymSpell std::unordered_map.</summary>
    /// <param name="input">The word being spell checked.</param>
    /// <param name="verbosity">The value controlling the quantity/closeness of the retuned suggestions.</param>
    /// <param name="maxEditDistance">The maximum edit distance between input and suggested words.</param>
    /// <returns>A List of SuggestItem object representing suggested correct spellings for the input word,
    /// sorted by edit distance, and secondarily by count frequency.</returns>
    std::vector<SuggestItem> Lookup(std::string input, Verbosity verbosity, int maxEditDistance);

    /// <summary>Find suggested spellings for a given input word.</summary>
    /// <param name="input">The word being spell checked.</param>
    /// <param name="verbosity">The value controlling the quantity/closeness of the retuned suggestions.</param>
    /// <param name="maxEditDistance">The maximum edit distance between input and suggested words.</param>
    /// <param name="includeUnknown">Include input word in suggestions, if no words within edit distance found.</param>
    /// <returns>A List of SuggestItem object representing suggested correct spellings for the input word,
    /// sorted by edit distance, and secondarily by count frequency.</returns>
    std::vector<SuggestItem> Lookup(std::string input, Verbosity verbosity, int maxEditDistance, bool includeUnknown);

private:
    //check whether all delete chars are present in the suggestion prefix in correct order, otherwise this is just a hash collision
    bool DeleteInSuggestionPrefix(std::string deleteSugg, int deleteLen, std::string suggestion, int suggestionLen);

    //create a non-unique wordlist from sample text
    //language independent (e.g. works with Chinese characters)
    std::vector<std::string> ParseWords(std::string text);

    //inexpensive and language independent: only deletes, no transposes + replaces + inserts
    //replaces and inserts are expensive and language dependent (Chinese has 70,000 Unicode Han characters)
    std::unordered_set<std::string>* Edits(std::string word, int editDistance, std::unordered_set<std::string>* deleteWords);

    std::unordered_set<std::string> EditsPrefix(std::string key);

    int GetstringHash(std::string s);

public:
    //######################

    //LookupCompound supports compound aware automatic spelling correction of multi-word input strings with three cases:
    //1. mistakenly inserted space into a correct word led to two incorrect terms
    //2. mistakenly omitted space between two correct words led to one incorrect combined term
    //3. multiple independent input terms with/without spelling errors

    /// <summary>Find suggested spellings for a multi-word input string (supports word splitting/merging).</summary>
    /// <param name="input">The string being spell checked.</param>
    /// <returns>A List of SuggestItem object representing suggested correct spellings for the input string.</returns>
    std::vector<SuggestItem> LookupCompound(std::string input);

    /// <summary>Find suggested spellings for a multi-word input string (supports word splitting/merging).</summary>
    /// <param name="input">The string being spell checked.</param>
    /// <param name="maxEditDistance">The maximum edit distance between input and suggested words.</param>
    /// <returns>A List of SuggestItem object representing suggested correct spellings for the input string.</returns>
    std::vector<SuggestItem> LookupCompound(std::string input, int editDistanceMax);

    //######

    //WordSegmentation divides a string into words by inserting missing spaces at the appropriate positions
    //misspelled words are corrected and do not affect segmentation
    //existing spaces are allowed and considered for optimum segmentation

    //SymSpell.WordSegmentation uses a novel approach *without* recursion.
    //https://medium.com/@wolfgarbe/fast-word-segmentation-for-noisy-text-2c2c41f9e8da
    //While each string of length n can be segmentend in 2^n−1 possible compositions https://en.wikipedia.org/wiki/Composition_(combinatorics)
    //SymSpell.WordSegmentation has a linear runtime O(n) to find the optimum composition

    //number of all words in the corpus used to generate the frequency std::unordered_map
    //this is used to calculate the word occurrence probability p from word counts c : p=c/N
    //N equals the sum of all counts c in the std::unordered_map only if the std::unordered_map is complete, but not if the std::unordered_map is truncated or filtered
    static const int64_t N = 1024908267229L;

    /// <summary>Find suggested spellings for a multi-word input string (supports word splitting/merging).</summary>
    /// <param name="input">The string being spell checked.</param>
    /// <returns>The word segmented string,
    /// the word segmented and spelling corrected string,
    /// the Edit distance sum between input string and corrected string,
    /// the Sum of word occurrence probabilities in log scale (a measure of how common and probable the corrected segmentation is).</returns>
    Info WordSegmentation(std::string input);

    /// <summary>Find suggested spellings for a multi-word input string (supports word splitting/merging).</summary>
    /// <param name="input">The string being spell checked.</param>
    /// <param name="maxEditDistance">The maximum edit distance between input and corrected words
    /// (0=no correction/segmentation only).</param>
    /// <returns>The word segmented string,
    /// the word segmented and spelling corrected string,
    /// the Edit distance sum between input string and corrected string,
    /// the Sum of word occurrence probabilities in log scale (a measure of how common and probable the corrected segmentation is).</returns>
    Info WordSegmentation(std::string input, int maxEditDistance);

    /// <summary>Find suggested spellings for a multi-word input string (supports word splitting/merging).</summary>
    /// <param name="input">The string being spell checked.</param>
    /// <param name="maxSegmentationWordLength">The maximum word length that should be considered.</param>
    /// <param name="maxEditDistance">The maximum edit distance between input and corrected words
    /// (0=no correction/segmentation only).</param>
    /// <returns>The word segmented string,
    /// the word segmented and spelling corrected string,
    /// the Edit distance sum between input string and corrected string,
    /// the Sum of word occurrence probabilities in log scale (a measure of how common and probable the corrected segmentation is).</returns>
    Info WordSegmentation(std::string input, int maxEditDistance, int maxSegmentationWordLength);
};