#pragma once

// #define UNICODE_SUPPORT
#define DEFAULT_SEPARATOR_CHAR XL(' ')
#define DEFAULT_MAX_EDIT_DISTANCE 2
#define DEFAULT_PREFIX_LENGTH 7
#define DEFAULT_COUNT_THRESHOLD 1
#define DEFAULT_INITIAL_CAPACITY 82765
#define DEFAULT_COMPACT_LEVEL 5
#include <algorithm>
#include <climits>
#include <cstdint>
#include <filesystem>
#include <istream>
#include <memory>
#include <ostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "include/Helpers.h"
#include "include/EditDistance.h"

// SymSpell supports compound splitting / decompounding of multi-word input strings with three cases:
// 1. mistakenly inserted space into a correct word led to two incorrect terms
// 2. mistakenly omitted space between two correct words led to one incorrect combined term
// 3. multiple independent input terms with/without spelling errors
namespace symspellcpppy
{
    class Info
    {
    private:
        xstring segmentedstring;
        xstring correctedstring;
        int distanceSum = 0;
        double probabilityLogSum = 0;

    public:
        void set(xstring seg, xstring cor, int d, double prob)
        {
            segmentedstring = std::move(seg);
            correctedstring = std::move(cor);
            distanceSum = d;
            probabilityLogSum = prob;
        };

        const xstring &getSegmented() const
        {
            return segmentedstring;
        };

        const xstring &getCorrected() const
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
    protected:
        int initialCapacity;
        int maxDictionaryEditDistance;
        int prefixLength;    // prefix length  5..7
        int countThreshold; // a threshold might be specified, when a term occurs so frequently in the corpus that it is considered a valid word for spelling correction
        unsigned char compactLevel;
        int compactMask;
        DistanceAlgorithm distanceAlgorithm = DistanceAlgorithm::DamerauOSADistance;
        int maxDictionaryWordLength; // maximum std::unordered_map term length
        // The index refers to map elements, whose addresses are stable across rehashes.
        DeleteIndex deletes;
        WordCounts words;
        std::unordered_map<xstring, int64_t> belowThresholdWords;

    public:
        int MaxDictionaryEditDistance() const;

        int PrefixLength() const;

        int MaxLength() const;

        int CountThreshold() const;

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

        SymSpell(const SymSpell &other);
        SymSpell &operator=(const SymSpell &other);
        SymSpell(SymSpell &&) noexcept = default;
        SymSpell &operator=(SymSpell &&) noexcept = default;

        bool CreateDictionaryEntry(const xstring &key, int64_t count, const std::shared_ptr<SuggestionStage> &staging = nullptr);

        bool DeleteDictionaryEntry(const xstring &key);

        std::unordered_map<xstring, int64_t> bigrams;
        int64_t bigramCountMin = INT64_MAX;

        /// <summary>Load multiple dictionary entries from a file of word/frequency count pairs</summary>
        /// <remarks>Merges with any dictionary data already loaded.</remarks>
        /// <param name="corpus">The path+filename of the file.</param>
        /// <param name="termIndex">The column position of the word.</param>
        /// <param name="countIndex">The column position of the frequency count.</param>
        /// <param name="separatorChars">Separator characters between term(s) and count.</param>
        /// <returns>True if file loaded, or false if file not found.</returns>
        bool LoadBigramDictionary(const std::filesystem::path &corpus, int termIndex, int countIndex,
                                  xchar separatorChars = DEFAULT_SEPARATOR_CHAR);

        bool LoadBigramDictionary(std::istream &corpusStream, int termIndex, int countIndex,
                                  xchar separatorChars = DEFAULT_SEPARATOR_CHAR);

        /// <summary>Load multiple dictionary entries from a file of word/frequency count pairs</summary>
        /// <remarks>Merges with any dictionary data already loaded.</remarks>
        /// <param name="corpus">The path+filename of the file.</param>
        /// <param name="termIndex">The column position of the word.</param>
        /// <param name="countIndex">The column position of the frequency count.</param>
        /// <param name="separatorChars">Separator characters between term(s) and count.</param>
        /// <returns>True if file loaded, or false if file not found.</returns>
        bool LoadDictionary(const std::filesystem::path &corpus, int termIndex, int countIndex,
                            xchar separatorChars = DEFAULT_SEPARATOR_CHAR);

        bool LoadDictionary(std::istream &corpusStream, int termIndex, int countIndex,
                            xchar separatorChars = DEFAULT_SEPARATOR_CHAR);

        /// <summary>Load multiple dictionary words from a file containing plain text.</summary>
        /// <remarks>Merges with any dictionary data already loaded.</remarks>
        /// <param name="corpus">The path+filename of the file.</param>
        /// <returns>True if file loaded, or false if file not found.</returns>
        bool CreateDictionary(const std::filesystem::path &corpus);

        bool CreateDictionary(std::istream &corpusStream);

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
        std::vector<SuggestItem> Lookup(const xstring &input, Verbosity verbosity);

        /// <summary>Find suggested spellings for a given input word, using the maximum
        /// edit distance specified during construction of the SymSpell dictionary.</summary>
        /// <param name="input">The word being spell checked.</param>
        /// <param name="verbosity">The value controlling the quantity/closeness of the retuned suggestions.</param>
        /// <param name="maxEditDistance">The maximum edit distance between input and suggested words.</param>
        /// <returns>A List of SuggestItem object representing suggested correct spellings for the input word,
        /// sorted by edit distance, and secondarily by count frequency.</returns>
        std::vector<SuggestItem> Lookup(const xstring &input, Verbosity verbosity, int maxEditDistance);

        /// <summary>Find suggested spellings for a given input word.</summary>
        /// <param name="input">The word being spell checked.</param>
        /// <param name="verbosity">The value controlling the quantity/closeness of the retuned suggestions.</param>
        /// <param name="maxEditDistance">The maximum edit distance between input and suggested words.</param>
        /// <param name="includeUnknown">Include input word in suggestions, if no words within edit distance found.</param>
        /// <returns>A List of SuggestItem object representing suggested correct spellings for the input word,
        /// sorted by edit distance, and secondarily by count frequency.</returns>
        std::vector<SuggestItem> Lookup(const xstring &input, Verbosity verbosity, int maxEditDistance, bool includeUnknown);

        /// <summary>Find suggested spellings for a given input word.</summary>
        /// <param name="input">The word being spell checked.</param>
        /// <param name="verbosity">The value controlling the quantity/closeness of the retuned suggestions.</param>
        /// <param name="maxEditDistance">The maximum edit distance between input and suggested words.</param>
        /// <param name="includeUnknown">Include input word in suggestions, if no words within edit distance found.</param>
        /// <param name="transfer_casing"> Lower case the word or not
        /// <returns>A List of SuggestItem object representing suggested correct spellings for the input word,
        /// sorted by edit distance, and secondarily by count frequency.</returns>
        std::vector<SuggestItem> Lookup(const xstring &input, Verbosity verbosity, int maxEditDistance, bool includeUnknown, bool transferCasing);

    private:
        bool
        DeleteInSuggestionPrefix(const xstring &deleteSugg, int deleteLen, const xstring &suggestion, int suggestionLen) const;

        static std::vector<xstring> ParseWords(const xstring &text);

        std::shared_ptr<std::unordered_set<xstring>>
        Edits(const xstring &word, int editDistance, std::shared_ptr<std::unordered_set<xstring>> deleteWords);

        std::shared_ptr<std::unordered_set<xstring>> EditsPrefix(const xstring &key);

        int GetstringHash(const xstring &s) const;

    public:
        // ######################

        // LookupCompound supports compound aware automatic spelling correction of multi-word input strings with three cases:
        // 1. mistakenly inserted space into a correct word led to two incorrect terms
        // 2. mistakenly omitted space between two correct words led to one incorrect combined term
        // 3. multiple independent input terms with/without spelling errors

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

        // ######

        // WordSegmentation divides a string into words by inserting missing spaces at the appropriate positions
        // misspelled words are corrected and do not affect segmentation
        // existing spaces are allowed and considered for optimum segmentation

        // SymSpell.WordSegmentation uses a novel approach *without* recursion.
        // https://medium.com/@wolfgarbe/fast-word-segmentation-for-noisy-text-2c2c41f9e8da
        // While each string of length n can be segmentend in 2^n−1 possible compositions https://en.wikipedia.org/wiki/Composition_(combinatorics)
        // SymSpell.WordSegmentation has a linear runtime O(n) to find the optimum composition

        // number of all words in the corpus used to generate the frequency dictionary
        // this is used to calculate the word occurrence probability p from word counts c : p=c/N
        // N equals the sum of all counts c in the dictionary only if the dictionary is complete, but not if the dictionary is truncated or filtered
        static const int64_t N = 1024908267229LL;

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

        void Save(std::ostream &stream) const;
        void Load(std::istream &stream);
    };
}
