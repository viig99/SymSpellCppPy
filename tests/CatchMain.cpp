#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"
#include "../library.h"

using namespace symspellcpppy;

TEST_CASE("Testing English", "[english]") {
    const int initialCapacity = 82765;
    const int maxEditDistance = 2;
    const int prefixLength = 3;

    SECTION("Do Word Segmentation") {
        SymSpell symSpell(initialCapacity, maxEditDistance, prefixLength);
        symSpell.LoadDictionary("../resources/frequency_dictionary_en_82_765.txt", 0, 1, XL(' '));
        std::unordered_map<xstring, xstring> sentences = {
                {XL("thequickbrownfoxjumpsoverthelazydog"),                                                XL("they quick brown fox jumps over therapy dog")},
                {XL("itwasabrightcolddayinaprilandtheclockswerestrikingthirteen"),                         XL("it was bright holiday in april another clocks were striking thirteen")},
                {XL("itwasthebestoftimesitwastheworstoftimesitwastheageofwisdomitwastheageoffoolishness"), XL("it waste best of times it waste worst of times it was thereof wisdom it was thereof foolishness")}
        };

        for (auto &sentence : sentences) {
            Info results = symSpell.WordSegmentation(sentence.first);
            REQUIRE(results.getCorrected() == sentence.second);
        }
    }

    SECTION("Do Spell Correction") {
        SymSpell symSpell(initialCapacity, maxEditDistance, prefixLength);
        symSpell.LoadDictionary("../resources/frequency_dictionary_en_82_765.txt", 0, 1, XL(' '));
        std::unordered_map<xstring, xstring> words = {
                {XL("tke"),          XL("take")},
                {XL("abolution"),    XL("abolition")},
                {XL("intermedaite"), XL("intermediate")}
        };

        for (auto &word : words) {
            std::vector<SuggestItem> results = symSpell.Lookup(word.first, Verbosity::Closest);
            REQUIRE(results[0].term == word.second);
        }
    }

    SECTION("Do Spell Correction With MaxEditDistance=2") {
        SymSpell symSpell(initialCapacity, maxEditDistance, prefixLength);
        symSpell.LoadDictionary("../resources/frequency_dictionary_en_82_765.txt", 0, 1, XL(' '));
        std::unordered_map<xstring, xstring> words_within_distance = {
                {XL("tke"),     XL("take")},
                {XL("extrine"), XL("extreme")}
        };

        std::unordered_map<xstring, xstring> words_far_distance = {
                {XL("elipnaht"),   XL("elephant")},
                {XL("aotocrasie"), XL("autocracy")}
        };

        for (auto &word : words_within_distance) {
            std::vector<SuggestItem> results = symSpell.Lookup(word.first, Verbosity::Closest, 2);
            REQUIRE(results[0].term == word.second);
        }

        for (auto &word : words_far_distance) {
            std::vector<SuggestItem> results = symSpell.Lookup(word.first, Verbosity::Closest, 2);
            REQUIRE(results.empty());
        }
    }

    SECTION("Correct Compound Mistakes") {
        SymSpell symSpell(initialCapacity, maxEditDistance, prefixLength);
        symSpell.LoadDictionary("../resources/frequency_dictionary_en_82_765.txt", 0, 1, XL(' '));
        std::unordered_map<xstring, xstring> compunded_sentences = {
                {XL("whereis th elove hehad dated forImuch of thepast who couqdn'tread in sixthgrade and ins pired him"), XL("whereas to love head dated for much of theist who couldn't read in sixth grade and inspired him")},
                {XL("in te dhird qarter oflast jear he hadlearned ofca sekretplan"),                                      XL("in to third quarter of last year he had learned of a secret plan")},
                {XL("the bigjest playrs in te strogsommer film slatew ith plety of funn"),                                XL("they biggest players in to strong summer film slate with plenty of fun")}
        };

        for (auto &sentence : compunded_sentences) {
            std::vector<SuggestItem> results = symSpell.LookupCompound(sentence.first);
            REQUIRE(results[0].term == sentence.second);
        }
    }

    SECTION("Check top verbosity") {
        SymSpell symSpellcustom(initialCapacity, maxEditDistance, prefixLength);
        symSpellcustom.LoadDictionary("../resources/frequency_dictionary_en_test_verbosity.txt", 0, 1, XL(' '));
        std::vector<SuggestItem> results = symSpellcustom.Lookup(XL("stream"), Verbosity::Top, 2);
        REQUIRE(1 == results.size());
        REQUIRE(XL("streamc") == results[0].term);
    }

    SECTION("Check all verbosity") {
        SymSpell symSpellcustom(initialCapacity, maxEditDistance, prefixLength);
        symSpellcustom.LoadDictionary("../resources/frequency_dictionary_en_test_verbosity.txt", 0, 1, XL(' '));
        std::vector<SuggestItem> results = symSpellcustom.Lookup(XL("stream"), Verbosity::All, 2);
        REQUIRE(2 == results.size());
    }

    SECTION("check custom entry of dictionary") {
        SymSpell symSpellcustom(100, maxEditDistance, prefixLength);
        SuggestionStage staging(100);
        symSpellcustom.CreateDictionaryEntry(XL("take"), 4, &staging);
        std::vector<SuggestItem> results = symSpellcustom.Lookup(XL("take"), Verbosity::Closest, 2);
        REQUIRE(XL("take") == results[0].term);
    }
}