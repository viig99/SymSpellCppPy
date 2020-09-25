//
// Created by vigi99 on 25/09/20.
//

#pragma once

#include "iostream"

class Helpers {
public:
    static int NullDistanceResults(const std::string& string1, const std::string& string2, double maxDistance) {
        if (string1.empty())
            return (string2.empty()) ? 0 : (string2.size() <= maxDistance) ? string2.size() : -1;
        return (string1.size() <= maxDistance) ? string1.size() : -1;
    }

    static int NullSimilarityResults(const std::string& string1, const std::string& string2, double minSimilarity) {
        return (string1.empty() && string2.empty()) ? 1 : (0 <= minSimilarity) ? 0 : -1;
    }

    static void PrefixSuffixPrep(std::string string1, std::string string2, int &len1, int &len2, int &start) {
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
};
