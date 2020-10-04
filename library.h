#pragma once

//#define UNICODE_SUPPORT
#define DEFAULT_SEPARATOR_CHAR XL(' ')
#define DEFAULT_MAX_EDIT_DISTANCE 2
#define DEFAULT_PREFIX_LENGTH 7
#define DEFAULT_COUNT_THRESHOLD 1
#define DEFAULT_INITIAL_CAPACITY 82765
#define DEFAULT_COMPACT_LEVEL 5
#define min3(a, b, c) (min(a, min(b, c)))
#define MAXINT LLONG_MAX
#define M
#define MAXLONG MAXINT

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
#include "include/Defines.h"
#include "include/Helpers.h"
#include "include/EditDistance.h"
#include "cereal/types/unordered_map.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/archives/binary.hpp"
#include "cereal/cereal.hpp"
#include "fstream"

// SymSpell supports compound splitting / decompounding of multi-word input strings with three cases:
// 1. mistakenly inserted space into a correct word led to two incorrect terms
// 2. mistakenly omitted space between two correct words led to one incorrect combined term
// 3. multiple independent input terms with/without spelling errors
namespace symspellcpppy {
    static inline void ltrim(xstring &s) {

        s.erase(s.begin(), find_if(s.begin(), s.end(), [](xchar ch) {
            return !isspace(ch);
        }));
    }

    static inline void rtrim(xstring &s) {
        s.erase(find_if(s.rbegin(), s.rend(), [](xchar ch) {
            return !isspace(ch);
        }).base(), s.end());
    }

    static inline void trim(xstring &s) {
        ltrim(s);
        rtrim(s);
    }

    class Info {
    private:

        xstring segmentedstring;
        xstring correctedstring;
        int distanceSum;
        double probabilityLogSum;
    public:
        void set(xstring &seg, xstring &cor, int d, double prob) {
            segmentedstring = seg;
            correctedstring = cor;
            distanceSum = d;
            probabilityLogSum = prob;
        };

        xstring getSegmented() const {
            return segmentedstring;
        };

        xstring getCorrected() const {
            return correctedstring;
        };

        int getDistance() const {
            return distanceSum;
        };

        double getProbability() const {
            return probabilityLogSum;
        };
    };

    /// <summary>Controls the closeness/quantity of returned spelling suggestions.</summary>
    enum Verbosity {
        /// <summary>Top suggestion with the highest term frequency of the suggestions of smallest edit distance found.</summary>
        Top,
        /// <summary>All suggestions of smallest edit distance found, suggestions ordered by term frequency.</summary>
        Closest,
        /// <summary>All suggestions within maxEditDistance, suggestions ordered by edit distance
        /// , then by term frequency (slower, no early termination).</summary>
        All
    };

    class SymSpell {
    private:
        int initialCapacity;
        int maxDictionaryEditDistance;
        int prefixLength; //prefix length  5..7
        long countThreshold; //a threshold might be specified, when a term occurs so frequently in the corpus that it is considered a valid word for spelling correction
        int compactMask;
        DistanceAlgorithm distanceAlgorithm = DistanceAlgorithm::DamerauOSADistance;
        int maxDictionaryWordLength; //maximum std::unordered_map term length
        std::shared_ptr<std::unordered_map<int, std::vector<xstring>>> deletes;
        std::unordered_map<xstring, int64_t> words;
        std::unordered_map<xstring, int64_t> belowThresholdWords;

    public:
        int MaxDictionaryEditDistance() const;

        int PrefixLength() const;

        int MaxLength() const;

        long CountThreshold() const;

        int WordCount();

        int EntryCount();

        /// <summary>Create a new instanc of SymSpell.</summary>
        /// <remarks>Specifying ann accurate initialCapacity is not essential,
        /// but it can help speed up processing by alleviating the need for
        /// data restructuring as the size grows.</remarks>
        /// <param name="initialCapacity">The expected number of words in dictionary.</param>
        /// <param name="maxDictionaryEditDistance">Maximum edit distance for doing lookups.</param>
        /// <param name="prefixLength">The length of word prefixes used for spell checking..</param>
        /// <param name="countThreshold">The minimum frequency count for dictionary words to be considered correct spellings.</param>
        /// <param name="compactLevel">Degree of favoring lower memory use over speed (0=fastest,most memory, 16=slowest,least memory).</param>
        explicit SymSpell(int maxDictionaryEditDistance = DEFAULT_MAX_EDIT_DISTANCE,
                          int prefixLength = DEFAULT_PREFIX_LENGTH, int countThreshold = DEFAULT_COUNT_THRESHOLD,
                          int initialCapacity = DEFAULT_INITIAL_CAPACITY,
                          unsigned char compactLevel = DEFAULT_COMPACT_LEVEL);

        bool CreateDictionaryEntry(const xstring &key, int64_t count, const std::shared_ptr<SuggestionStage> &staging);

        bool DeleteDictionaryEntry(const xstring &key);

        std::unordered_map<xstring, long> bigrams;
        int64_t bigramCountMin = MAXLONG;

        /// <summary>Load multiple dictionary entries from a file of word/frequency count pairs</summary>
        /// <remarks>Merges with any dictionary data already loaded.</remarks>
        /// <param name="corpus">The path+filename of the file.</param>
        /// <param name="termIndex">The column position of the word.</param>
        /// <param name="countIndex">The column position of the frequency count.</param>
        /// <param name="separatorChars">Separator characters between term(s) and count.</param>
        /// <returns>True if file loaded, or false if file not found.</returns>
        bool LoadBigramDictionary(const std::string &corpus, int termIndex, int countIndex,
                                  xchar separatorChars = DEFAULT_SEPARATOR_CHAR);

        bool LoadBigramDictionary(xifstream &corpusStream, int termIndex, int countIndex,
                                  xchar separatorChars = DEFAULT_SEPARATOR_CHAR);

        /// <summary>Load multiple dictionary entries from a file of word/frequency count pairs</summary>
        /// <remarks>Merges with any dictionary data already loaded.</remarks>
        /// <param name="corpus">The path+filename of the file.</param>
        /// <param name="termIndex">The column position of the word.</param>
        /// <param name="countIndex">The column position of the frequency count.</param>
        /// <param name="separatorChars">Separator characters between term(s) and count.</param>
        /// <returns>True if file loaded, or false if file not found.</returns>
        bool LoadDictionary(const std::string &corpus, int termIndex, int countIndex,
                            xchar separatorChars = DEFAULT_SEPARATOR_CHAR);

        bool LoadDictionary(xifstream &corpusStream, int termIndex, int countIndex,
                            xchar separatorChars = DEFAULT_SEPARATOR_CHAR);

        /// <summary>Load multiple dictionary words from a file containing plain text.</summary>
        /// <remarks>Merges with any dictionary data already loaded.</remarks>
        /// <param name="corpus">The path+filename of the file.</param>
        /// <returns>True if file loaded, or false if file not found.</returns>
        bool CreateDictionary(const std::string &corpus);

        bool CreateDictionary(xifstream &corpusStream);

        /// <summary>Remove all below threshold words from the dictionary.</summary>
        /// <remarks>This can be used to reduce memory consumption after populating the dictionary from
        /// a corpus using CreateDictionary.</remarks>
        void PurgeBelowThresholdWords();

        void CommitStaged(const std::shared_ptr<SuggestionStage> &staging);

        /// <summary>Find suggested spellings for a given input word, using the maximum
        /// edit distance specified during construction of the SymSpell dictionary.</summary>
        /// <param name="input">The word being spell checked.</param>
        /// <param name="verbosity">The value controlling the quantity/closeness of the retuned suggestions.</param>
        /// <returns>A List of SuggestItem object representing suggested correct spellings for the input word,
        /// sorted by edit distance, and secondarily by count frequency.</returns>
        std::vector<SuggestItem> Lookup(xstring input, Verbosity verbosity);

        /// <summary>Find suggested spellings for a given input word, using the maximum
        /// edit distance specified during construction of the SymSpell dictionary.</summary>
        /// <param name="input">The word being spell checked.</param>
        /// <param name="verbosity">The value controlling the quantity/closeness of the retuned suggestions.</param>
        /// <param name="maxEditDistance">The maximum edit distance between input and suggested words.</param>
        /// <returns>A List of SuggestItem object representing suggested correct spellings for the input word,
        /// sorted by edit distance, and secondarily by count frequency.</returns>
        std::vector<SuggestItem> Lookup(xstring input, Verbosity verbosity, int maxEditDistance);

        /// <summary>Find suggested spellings for a given input word.</summary>
        /// <param name="input">The word being spell checked.</param>
        /// <param name="verbosity">The value controlling the quantity/closeness of the retuned suggestions.</param>
        /// <param name="maxEditDistance">The maximum edit distance between input and suggested words.</param>
        /// <param name="includeUnknown">Include input word in suggestions, if no words within edit distance found.</param>
        /// <returns>A List of SuggestItem object representing suggested correct spellings for the input word,
        /// sorted by edit distance, and secondarily by count frequency.</returns>
        std::vector<SuggestItem> Lookup(xstring input, Verbosity verbosity, int maxEditDistance, bool includeUnknown);

        /// <summary>Find suggested spellings for a given input word.</summary>
        /// <param name="input">The word being spell checked.</param>
        /// <param name="verbosity">The value controlling the quantity/closeness of the retuned suggestions.</param>
        /// <param name="maxEditDistance">The maximum edit distance between input and suggested words.</param>
        /// <param name="includeUnknown">Include input word in suggestions, if no words within edit distance found.</param>
        /// <param name="transfer_casing"> Lower case the word or not
        /// <returns>A List of SuggestItem object representing suggested correct spellings for the input word,
        /// sorted by edit distance, and secondarily by count frequency.</returns>
        std::vector<SuggestItem> Lookup(xstring input, Verbosity verbosity, int maxEditDistance, bool includeUnknown, bool transferCasing);

    private:
        bool
        DeleteInSuggestionPrefix(xstring deleteSugg, int deleteLen, xstring suggestion, int suggestionLen) const;

        static std::vector<xstring> ParseWords(const xstring &text);

        std::shared_ptr<std::unordered_set<xstring>>
        Edits(const xstring &word, int editDistance, std::shared_ptr<std::unordered_set<xstring>> deleteWords);

        std::shared_ptr<std::unordered_set<xstring>> EditsPrefix(xstring key);

        int GetstringHash(xstring s) const;

    public:
        //######################

        //LookupCompound supports compound aware automatic spelling correction of multi-word input strings with three cases:
        //1. mistakenly inserted space into a correct word led to two incorrect terms
        //2. mistakenly omitted space between two correct words led to one incorrect combined term
        //3. multiple independent input terms with/without spelling errors

        /// <summary>Find suggested spellings for a multi-word input string (supports word splitting/merging).</summary>
        /// <param name="input">The string being spell checked.</param>
        /// <returns>A List of SuggestItem object representing suggested correct spellings for the input string.</returns>
        std::vector<SuggestItem> LookupCompound(const xstring &input);

        /// <summary>Find suggested spellings for a multi-word input string (supports word splitting/merging).</summary>
        /// <param name="input">The string being spell checked.</param>
        /// <param name="maxEditDistance">The maximum edit distance between input and suggested words.</param>
        /// <returns>A List of SuggestItem object representing suggested correct spellings for the input string.</returns>
        std::vector<SuggestItem> LookupCompound(const xstring &input, int editDistanceMax);

        /// <summary>Find suggested spellings for a multi-word input string (supports word splitting/merging).</summary>
        /// <param name="input">The string being spell checked.</param>
        /// <param name="maxEditDistance">The maximum edit distance between input and suggested words.</param>
        /// <returns>A List of SuggestItem object representing suggested correct spellings for the input string.</returns>
        std::vector<SuggestItem> LookupCompound(const xstring &input, int editDistanceMax, bool transferCasing);

        //######

        //WordSegmentation divides a string into words by inserting missing spaces at the appropriate positions
        //misspelled words are corrected and do not affect segmentation
        //existing spaces are allowed and considered for optimum segmentation

        //SymSpell.WordSegmentation uses a novel approach *without* recursion.
        //https://medium.com/@wolfgarbe/fast-word-segmentation-for-noisy-text-2c2c41f9e8da
        //While each string of length n can be segmentend in 2^n−1 possible compositions https://en.wikipedia.org/wiki/Composition_(combinatorics)
        //SymSpell.WordSegmentation has a linear runtime O(n) to find the optimum composition

        //number of all words in the corpus used to generate the frequency dictionary
        //this is used to calculate the word occurrence probability p from word counts c : p=c/N
        //N equals the sum of all counts c in the dictionary only if the dictionary is complete, but not if the dictionary is truncated or filtered
        static const int64_t N = 1024908267229L;

        /// <summary>Find suggested spellings for a multi-word input string (supports word splitting/merging).</summary>
        /// <param name="input">The string being spell checked.</param>
        /// <returns>The word segmented string,
        /// the word segmented and spelling corrected string,
        /// the Edit distance sum between input string and corrected string,
        /// the Sum of word occurrence probabilities in log scale (a measure of how common and probable the corrected segmentation is).</returns>
        Info WordSegmentation(const xstring &input);

        /// <summary>Find suggested spellings for a multi-word input string (supports word splitting/merging).</summary>
        /// <param name="input">The string being spell checked.</param>
        /// <param name="maxEditDistance">The maximum edit distance between input and corrected words
        /// (0=no correction/segmentation only).</param>
        /// <returns>The word segmented string,
        /// the word segmented and spelling corrected string,
        /// the Edit distance sum between input string and corrected string,
        /// the Sum of word occurrence probabilities in log scale (a measure of how common and probable the corrected segmentation is).</returns>
        Info WordSegmentation(const xstring &input, int maxEditDistance);

        /// <summary>Find suggested spellings for a multi-word input string (supports word splitting/merging).</summary>
        /// <param name="input">The string being spell checked.</param>
        /// <param name="maxSegmentationWordLength">The maximum word length that should be considered.</param>
        /// <param name="maxEditDistance">The maximum edit distance between input and corrected words
        /// (0=no correction/segmentation only).</param>
        /// <returns>The word segmented string,
        /// the word segmented and spelling corrected string,
        /// the Edit distance sum between input string and corrected string,
        /// the Sum of word occurrence probabilities in log scale (a measure of how common and probable the corrected segmentation is).</returns>
        Info WordSegmentation(const xstring &input, int maxEditDistance, int maxSegmentationWordLength);


        template<class Archive>
        void serialize(Archive &ar) {
            ar(deletes, words, maxDictionaryWordLength);
        }
    };
}