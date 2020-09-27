//
// Created by vigi99 on 25/09/20.
//

#pragma once

#include "BaseDistance.h"
#include "BaseSimilarity.h"
#include "Helpers.h"
#include "Defines.h"
#include <vector>
#include <cmath>
#include <climits>
#include <stdexcept>

class Levenshtein : public BaseDistance, BaseSimilarity {
private:
    std::vector<int> baseChar1Costs;

public:

    Levenshtein() = default;

    explicit Levenshtein(int expectedMaxstringLength) {
        if (expectedMaxstringLength <= 0) throw std::invalid_argument("expectedMaxstringLength must be larger than 0");
        baseChar1Costs = std::vector<int>(expectedMaxstringLength, 0);
    }

    double Distance(xstring string1, xstring string2) override {
        if (string1.empty()) return string2.size();
        if (string2.empty()) return string1.size();

        if (string1.size() > string2.size()) {
            xstring t = string1;
            string1 = string2;
            string2 = t;
        }

        int len1, len2, start;
        Helpers::PrefixSuffixPrep(string1, string2, len1, len2, start);
        if (len1 == 0) return len2;

        return Distance(string1, string2, len1, len2, start,
                        (baseChar1Costs = (len2 <= baseChar1Costs.size()) ? baseChar1Costs : std::vector<int>(len2,
                                                                                                              0)));
    }

    double Distance(xstring string1, xstring string2, double maxDistance) override {
        if (string1.empty() || string2.empty()) return Helpers::NullDistanceResults(string1, string2, maxDistance);
        if (maxDistance <= 0) return (string1 == string2) ? 0 : -1;
        maxDistance = ceil(maxDistance);
        int iMaxDistance = (maxDistance <= INT_MAX) ? (int) maxDistance : INT_MAX;

        if (string1.size() > string2.size()) {
            xstring t = string1;
            string1 = string2;
            string2 = t;
        }
        if (string2.size() - string1.size() > iMaxDistance) return -1;

        int len1, len2, start;
        Helpers::PrefixSuffixPrep(string1, string2, len1, len2, start);
        if (len1 == 0) return (len2 <= iMaxDistance) ? len2 : -1;

        if (iMaxDistance < len2) {
            return Distance(string1, string2, len1, len2, start, iMaxDistance,
                            (baseChar1Costs = (len2 <= baseChar1Costs.size()) ? baseChar1Costs : std::vector<int>(len2,
                                                                                                                  0)));
        }
        return Distance(string1, string2, len1, len2, start,
                        (baseChar1Costs = (len2 <= baseChar1Costs.size()) ? baseChar1Costs : std::vector<int>(len2,
                                                                                                              0)));
    }

    double Similarity(xstring string1, xstring string2) override {
        if (string1.empty()) return (string2.empty()) ? 1 : 0;
        if (string2.empty()) return 0;

        if (string1.size() > string2.size()) {
            xstring t = string1;
            string1 = string2;
            string2 = t;
        }

        int len1, len2, start;
        Helpers::PrefixSuffixPrep(string1, string2, len1, len2, start);
        if (len1 == 0) return 1.0;

        return Helpers::ToSimilarity(Distance(string1, string2, len1, len2, start,
                                              (baseChar1Costs = (len2 <= baseChar1Costs.size()) ? baseChar1Costs
                                                                                                : std::vector<int>(len2,
                                                                                                                   0))),
                                     string2.size());
    }

    double Similarity(xstring string1, xstring string2, double minSimilarity) override {
        if (minSimilarity < 0 || minSimilarity > 1)
            throw std::invalid_argument("minSimilarity must be in range 0 to 1.0");
        if (string1.empty() || string2.empty()) return Helpers::NullSimilarityResults(string1, string2, minSimilarity);

        if (string1.size() > string2.size()) {
            xstring t = string1;
            string1 = string2;
            string2 = t;
        }

        int iMaxDistance = Helpers::ToDistance(minSimilarity, string2.size());
        if (string2.size() - string1.size() > iMaxDistance) return -1;
        if (iMaxDistance == 0) return (string1 == string2) ? 1 : -1;

        int len1, len2, start;
        Helpers::PrefixSuffixPrep(string1, string2, len1, len2, start);
        if (len1 == 0) return 1.0;

        if (iMaxDistance < len2) {
            return Helpers::ToSimilarity(Distance(string1, string2, len1, len2, start, iMaxDistance,
                                                  (baseChar1Costs = (len2 <= baseChar1Costs.size()) ? baseChar1Costs
                                                                                                    : std::vector<int>(
                                                                  len2, 0))), string2.size());
        }
        return Helpers::ToSimilarity(Distance(string1, string2, len1, len2, start,
                                              (baseChar1Costs = (len2 <= baseChar1Costs.size()) ? baseChar1Costs
                                                                                                : std::vector<int>(len2,
                                                                                                                   0))),
                                     string2.size());
    }

    static int
    Distance(xstring string1, xstring string2, int len1, int len2, int start, std::vector<int> &char1Costs) {
        for (int j = 0; j < len2;) char1Costs[j] = ++j;
        int currentCharCost = 0;
        if (start == 0) {
            for (int i = 0; i < len1; ++i) {
                int leftCharCost, aboveCharCost;
                leftCharCost = aboveCharCost = i;
                xchar char1 = string1[i];
                for (int j = 0; j < len2; ++j) {
                    currentCharCost = leftCharCost; // cost on diagonal (substitution)
                    leftCharCost = char1Costs[j];
                    if (string2[j] != char1) {
                        if (aboveCharCost < currentCharCost) currentCharCost = aboveCharCost; // deletion
                        if (leftCharCost < currentCharCost) currentCharCost = leftCharCost; // insertion
                        ++currentCharCost;
                    }
                    char1Costs[j] = aboveCharCost = currentCharCost;
                }
            }
        } else {
            for (int i = 0; i < len1; ++i) {
                int leftCharCost, aboveCharCost;
                leftCharCost = aboveCharCost = i;
                xchar char1 = string1[start + i];
                for (int j = 0; j < len2; ++j) {
                    currentCharCost = leftCharCost; // cost on diagonal (substitution)
                    leftCharCost = char1Costs[j];
                    if (string2[start + j] != char1) {
                        if (aboveCharCost < currentCharCost) currentCharCost = aboveCharCost; // deletion
                        if (leftCharCost < currentCharCost) currentCharCost = leftCharCost; // insertion
                        ++currentCharCost;
                    }
                    char1Costs[j] = aboveCharCost = currentCharCost;
                }
            }
        }
        return currentCharCost;
    }

    static int Distance(xstring string1, xstring string2, int len1, int len2, int start, int maxDistance,
                        std::vector<int> &char1Costs) {
        int i, j;
        for (j = 0; j < maxDistance;) char1Costs[j] = ++j;
        for (; j < len2;) char1Costs[j++] = maxDistance + 1;
        int lenDiff = len2 - len1;
        int jStartOffset = maxDistance - lenDiff;
        int jStart = 0;
        int jEnd = maxDistance;
        int currentCost = 0;
        if (start == 0) {
            for (i = 0; i < len1; ++i) {
                xchar char1 = string1[i];
                int prevChar1Cost, aboveCharCost;
                prevChar1Cost = aboveCharCost = i;
                jStart += (i > jStartOffset) ? 1 : 0;
                jEnd += (jEnd < len2) ? 1 : 0;
                for (j = jStart; j < jEnd; ++j) {
                    currentCost = prevChar1Cost; // cost on diagonal (substitution)
                    prevChar1Cost = char1Costs[j];
                    if (string2[j] != char1) {
                        if (aboveCharCost < currentCost) currentCost = aboveCharCost; // deletion
                        if (prevChar1Cost < currentCost) currentCost = prevChar1Cost;   // insertion
                        ++currentCost;
                    }
                    char1Costs[j] = aboveCharCost = currentCost;
                }
                if (char1Costs[i + lenDiff] > maxDistance) return -1;
            }
        } else {
            for (i = 0; i < len1; ++i) {
                xchar char1 = string1[start + i];
                int prevChar1Cost, aboveCharCost;
                prevChar1Cost = aboveCharCost = i;
                jStart += (i > jStartOffset) ? 1 : 0;
                jEnd += (jEnd < len2) ? 1 : 0;
                for (j = jStart; j < jEnd; ++j) {
                    currentCost = prevChar1Cost; // cost on diagonal (substitution)
                    prevChar1Cost = char1Costs[j];
                    if (string2[start + j] != char1) {
                        if (aboveCharCost < currentCost) currentCost = aboveCharCost; // deletion
                        if (prevChar1Cost < currentCost) currentCost = prevChar1Cost;   // insertion
                        ++currentCost;
                    }
                    char1Costs[j] = aboveCharCost = currentCost;
                }
                if (char1Costs[i + lenDiff] > maxDistance) return -1;
            }
        }
        return (currentCost <= maxDistance) ? currentCost : -1;
    }
};