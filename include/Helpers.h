//
// Created by vigi99 on 25/09/20.
//

#pragma once

#include <unordered_map>
#include <utility>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include "iostream"
#include "Defines.h"
#include <difflib.h>

class Helpers {
public:
    static int NullDistanceResults(const xstring &string1, const xstring &string2, double maxDistance) {
        if (string1.empty())
            return (string2.empty()) ? 0 : (string2.size() <= maxDistance) ? string2.size() : -1;
        return (string1.size() <= maxDistance) ? string1.size() : -1;
    }

    static int NullSimilarityResults(const xstring &string1, const xstring &string2, double minSimilarity) {
        return (string1.empty() && string2.empty()) ? 1 : (0 <= minSimilarity) ? 0 : -1;
    }

    static void PrefixSuffixPrep(const xstring& string1, const xstring& string2, int &len1, int &len2, int &start) {
        len2 = string2.size();
        len1 = string1.size(); // this is also the minimum length of the two strings
        // suffix common to both strings can be ignored
        while (len1 != 0 && string1[len1 - 1] == string2[len2 - 1]) {
            len1 = len1 - 1;
            len2 = len2 - 1;
        }
        // prefix common to both strings can be ignored
        start = 0;
        while (start != len1 && string1[start] == string2[start]) start++;
        if (start != 0) {
            len2 -= start; // length of the part excluding common prefix and suffix
            len1 -= start;
        }
    }

    static double ToSimilarity(int distance, int length) {
        return (distance < 0) ? -1 : 1 - (distance / (double) length);
    }

    static int ToDistance(double similarity, int length) {
        return (int) ((length * (1 - similarity)) + .0000000001);
    }

    static int CompareTo(int64_t mainValue, int64_t compareValue) {
        if (mainValue == compareValue)
            return 0;
        else if (mainValue > compareValue)
            return 1;
        else
            return -1;
    }

    static xstring string_lower(const xstring& a) {
        xstring a_lower = a;
        std::transform(a.begin(), a.end(), a_lower.begin(), symspellcpppy::unicode::lower);
        return a_lower;
    }

    static xstring transfer_casing_for_similar_text(const xstring &source, const xstring &target) {
        namespace unicode = symspellcpppy::unicode;
        if (source.empty() || target.empty())
            return target;

        // SequenceMatcher holds a reference to its inputs.
        const auto lower_source = string_lower(source);
        auto matcher = difflib::MakeSequenceMatcher(lower_source, target);
        xstring result;
        result.reserve(target.size());

        for (const auto &[tag, source_begin, source_end, target_begin, target_end] : matcher.get_opcodes()) {
            if (tag == "equal") {
                result.append(source, source_begin, source_end - source_begin);
            } else if (tag == "insert") {
                // At a word boundary, inherit the case of the next source character.
                auto neighbor = source_begin;
                if (neighbor > 0 && (neighbor == source.size() || !unicode::is_space(source[neighbor - 1])))
                    --neighbor;
                const bool upper = unicode::is_upper(source[neighbor]);
                for (auto i = target_begin; i < target_end; ++i)
                    result.push_back(upper ? unicode::upper(target[i]) : unicode::lower(target[i]));
            } else if (tag == "replace") {
                for (auto i = target_begin; i < target_end; ++i) {
                    const auto source_index = std::min(source_begin + i - target_begin, source_end - 1);
                    result.push_back(unicode::is_upper(source[source_index])
                                         ? unicode::upper(target[i]) : unicode::lower(target[i]));
                }
            }
        }

        return result;
    }

};

template<class T>
class ChunkArray {
private:
    const int ChunkSize = 4096; //this must be a power of 2, otherwise can't optimize Row and Col functions
    const int DivShift = 12; // number of bits to shift right to do division by ChunkSize (the bit position of ChunkSize)
    int Row(unsigned int index) { return index >> DivShift; } // same as index / ChunkSize
    int Col(unsigned int index) { return index & (ChunkSize - 1); } //same as index % ChunkSize
    int Capacity() { return Values.size() * ChunkSize; }

public:
    std::vector<std::vector<T>> Values;
    int Count;

    ChunkArray() {
        Count = 0;
    }

    void Reserve(int initialCapacity) {
        int chunks = (initialCapacity + ChunkSize - 1) / ChunkSize;
        Values.resize(chunks);
        for (int i = 0; i < chunks; ++i) {
            Values[i].resize(ChunkSize);
        }
    }

    int Add(T &value) {
        if (Count == Capacity()) {
            Values.push_back(std::vector<T>());
            Values[Values.size() - 1].resize(ChunkSize);
        }

        int row = Row(Count);
        int col = Col(Count);

        Values[row][col] = value;
        return Count++;
    }

    void Clear() {
        Count = 0;
    }

    T &At(unsigned int index) {
        return Values[Row(index)][Col(index)];
    }

    void Set(unsigned int index, T &value) {
        Values[Row(index)][Col(index)] = value;
    }
};

using WordCounts = std::unordered_map<xstring, int64_t>;
using DeleteIndex = std::unordered_map<int, std::vector<const WordCounts::value_type *>>;

class SuggestionStage {
    using IndexedWord = const WordCounts::value_type *;

    struct Node {
        const IndexedWord *word;
        int next;
    };

    struct Entry {
        int count = 0;
        int first = -1;
    };

    std::unordered_map<int, Entry> deletes;
    std::unordered_map<xstring, IndexedWord> stagedWords;
    ChunkArray<Node> nodes;

public:
    explicit SuggestionStage(int initialCapacity) {
        deletes.reserve(initialCapacity);
        nodes.Reserve(initialCapacity * 2);
    }

    int DeleteCount() const { return deletes.size(); }

    const IndexedWord *TrackWord(const xstring &word) { return &stagedWords[word]; }

    void Add(int deleteHash, const IndexedWord *word) {
        auto &entry = deletes[deleteHash];
        Node node{word, entry.first};
        entry.first = nodes.Add(node);
        ++entry.count;
    }

    void CommitTo(DeleteIndex &index, const WordCounts &words) {
        // Resolve terms once so removing a word before commit cannot leave a stale pointer.
        for (auto &[key, word] : stagedWords) {
            const auto found = words.find(key);
            word = found == words.end() ? nullptr : &*found;
        }
        for (const auto &[hash, entry] : deletes) {
            auto &suggestions = index[hash];
            suggestions.reserve(suggestions.size() + entry.count);
            for (auto next = entry.first; next >= 0; ) {
                const auto &node = nodes.At(next);
                if (*node.word)
                    suggestions.push_back(*node.word);
                next = node.next;
            }
            if (suggestions.empty())
                index.erase(hash);
        }
        deletes.clear();
        nodes.Clear();
        stagedWords.clear();
    }
};


class SuggestItem {
public:
    xstring term;
    int distance = 0;
    int64_t count = 0;

    SuggestItem() = default;


    SuggestItem(xstring term, int distance, int64_t count) {
        this->term = std::move(term);
        this->distance = distance;
        this->count = count;
    }

    int CompareTo(const SuggestItem &other) const {
        int disCom = Helpers::CompareTo(this->distance, other.distance);
        if (disCom != 0)
            return disCom;
        int cntCom = Helpers::CompareTo(other.count, this->count);
        if (cntCom != 0)
            return cntCom;
        return this->term.compare(other.term);
    }

    bool Equals(const SuggestItem &obj) const {
        return this->term == obj.term && this->distance == obj.distance && this->count == obj.count;
    }

    int GetHashCode() const {
        return std::hash<xstring>{}(this->term);
    }

    std::string Tostring() const {
        return "{" + symspellcpppy::unicode::encode(term) + ", " + std::to_string(distance) +
               ", " + std::to_string(count) + "}";
    }

    static bool compare(const SuggestItem &s1, const SuggestItem &s2) {
        return s1.CompareTo(s2) < 0;
    }

    void set(const SuggestItem &exam) {
        this->term = exam.term;
        this->distance = exam.distance;
        this->count = exam.count;
    }
};
