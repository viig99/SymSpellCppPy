#pragma once

//#define UNICODE_SUPPORT
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

    enum Verbosity {
        Top,
        Closest,
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
        std::unordered_map<int, std::vector<xstring>> *deletes = nullptr;
        std::unordered_map<xstring, int64_t> words;
        std::unordered_map<xstring, int64_t> belowThresholdWords;

    public:
        int MaxDictionaryEditDistance() const;

        int PrefixLength() const;

        int MaxLength() const;

        long CountThreshold() const;

        int WordCount();

        int EntryCount();

        explicit SymSpell(int initialCapacity = DEFAULT_INITIAL_CAPACITY,
                          int maxDictionaryEditDistance = DEFAULT_MAX_EDIT_DISTANCE,
                          int prefixLength = DEFAULT_PREFIX_LENGTH, int countThreshold = DEFAULT_COUNT_THRESHOLD,
                          unsigned char compactLevel = DEFAULT_COMPACT_LEVEL);

        bool CreateDictionaryEntry(const xstring &key, int64_t count, SuggestionStage *staging);

        std::unordered_map<xstring, long> bigrams;
        int64_t bigramCountMin = MAXLONG;

        bool LoadBigramDictionary(const std::string &corpus, int termIndex, int countIndex,
                                  xchar separatorChars = DEFAULT_SEPARATOR_CHAR);

        bool LoadBigramDictionary(xifstream &corpusStream, int termIndex, int countIndex,
                                  xchar separatorChars = DEFAULT_SEPARATOR_CHAR);

        bool LoadDictionary(const std::string &corpus, int termIndex, int countIndex,
                            xchar separatorChars = DEFAULT_SEPARATOR_CHAR);

        bool LoadDictionary(xifstream &corpusStream, int termIndex, int countIndex,
                            xchar separatorChars = DEFAULT_SEPARATOR_CHAR);

        bool CreateDictionary(const std::string &corpus);

        bool CreateDictionary(xifstream &corpusStream);

        void PurgeBelowThresholdWords();

        void CommitStaged(SuggestionStage *staging);

        std::vector<SuggestItem> Lookup(xstring input, Verbosity verbosity);

        std::vector<SuggestItem> Lookup(xstring input, Verbosity verbosity, int maxEditDistance);

        std::vector<SuggestItem> Lookup(xstring input, Verbosity verbosity, int maxEditDistance, bool includeUnknown);

        std::vector<xstring> LookupTerm(xstring input, Verbosity verbosity);

        std::vector<xstring> LookupTerm(xstring input, Verbosity verbosity, int maxEditDistance);

        std::vector<xstring> LookupTerm(xstring input, Verbosity verbosity, int maxEditDistance, bool includeUnknown);

    private:
        bool
        DeleteInSuggestionPrefix(xstring deleteSugg, int deleteLen, xstring suggestion, int suggestionLen) const;

        static std::vector<xstring> ParseWords(const xstring &text);

        std::unordered_set<xstring> *
        Edits(const xstring &word, int editDistance, std::unordered_set<xstring> *deleteWords);

        std::unordered_set<xstring> EditsPrefix(xstring key);

        int GetstringHash(xstring s) const;

    public:
        std::vector<SuggestItem> LookupCompound(const xstring &input);

        std::vector<SuggestItem> LookupCompound(const xstring &input, int editDistanceMax);

        std::vector<xstring> LookupCompoundTerm(const xstring &input);

        std::vector<xstring> LookupCompoundTerm(const xstring &input, int editDistanceMax);

        static const int64_t N = 1024908267229L;

        Info WordSegmentation(const xstring &input);

        Info WordSegmentation(const xstring &input, int maxEditDistance);

        Info WordSegmentation(const xstring &input, int maxEditDistance, int maxSegmentationWordLength);
    };
}