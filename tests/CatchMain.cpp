#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"
#include "../library.h"

TEST_CASE("Testing English", "[english]") {
    const int initialCapacity = 82765;
    const int maxEditDistance = 2;
    const int prefixLength = 3;
    SymSpell symSpell(initialCapacity, maxEditDistance, prefixLength);

    int start = clock();
    symSpell.LoadDictionary("../resources/frequency_dictionary_en_82_765.txt", 0, 1, XL(' '));
    int end = clock();
    auto time = (float) ((end - start) / (CLOCKS_PER_SEC / 1000));
    INFO(XL("Library loaded: ") << time << XL(" ms"));

    SECTION("Word Segmentation") {
        std::unordered_map<std::string, std::string> sentences = {
                {"thequickbrownfoxjumpsoverthelazydog",                                                "they quick brown fox jumps over therapy dog"},
                {"itwasabrightcolddayinaprilandtheclockswerestrikingthirteen",                         "it was bright holiday in april another clocks were striking thirteen"},
                {"itwasthebestoftimesitwastheworstoftimesitwastheageofwisdomitwastheageoffoolishness", "it waste best of times it waste worst of times it was thereof wisdom it was thereof foolishness"}
        };

        for (auto &sentence : sentences) {
            Info results = symSpell.WordSegmentation(sentence.first);
            REQUIRE(results.getCorrected() == sentence.second);
        }
    }

    SECTION("SPELL CORRECTION") {
        std::unordered_map<std::string, std::string> words = {
                {"tke",          "take"},
                {"abolution",    "abolition"},
                {"intermedaite", "intermediate"}
        };

        for (auto &word : words) {
            std::vector<SuggestItem> results = symSpell.Lookup(word.first, Verbosity::Closest);
            REQUIRE(results[0].term == word.second);
        }
    }

    SECTION("COMPOUND MISTAKES") {
        std::unordered_map<std::string, std::string> compunded_sentences = {
                {"whereis th elove hehad dated forImuch of thepast who couqdn'tread in sixthgrade and ins pired him", "whereas to love head dated for much of theist who couldn't read in sixth grade and inspired him"},
                {"in te dhird qarter oflast jear he hadlearned ofca sekretplan",                                      "in to third quarter of last year he had learned of a secret plan"},
                {"the bigjest playrs in te strogsommer film slatew ith plety of funn",                                "they biggest players in to strong summer film slate with plenty of fun"}
        };

        for (auto &sentence : compunded_sentences) {
            std::vector<SuggestItem> results = symSpell.LookupCompound(sentence.first);
            REQUIRE(results[0].term == sentence.second);
        }
    }
}
