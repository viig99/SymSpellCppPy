#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"
#include "../library.h"

using namespace symspellcpppy;

TEST_CASE("Testing English", "[english]") {
    const int maxEditDistance = 2;
    const int prefixLength = 3;

    SECTION("Do Word Segmentation") {
        SymSpell symSpell(maxEditDistance, prefixLength);
        symSpell.LoadDictionary("../resources/frequency_dictionary_en_82_765.txt", 0, 1, XL(' '));
        std::unordered_map<xstring, xstring> sentences = {
                {XL("thequickbrownfoxjumpsoverthelazydog"),                                                XL("the quick brown fox jumps over the lazy dog")},
                {XL("itwasabrightcolddayinaprilandtheclockswerestrikingthirteen"),                         XL("it was bright holiday in april and the clocks were striking thirteen")},
                {XL("itwasthebestoftimesitwastheworstoftimesitwastheageofwisdomitwastheageoffoolishness"), XL("iowa the best of times it waste worst of times it was thereof wisdom it was thereof foolishness")}
        };

        for (auto &sentence : sentences) {
            auto results = symSpell.WordSegmentation(sentence.first);
            REQUIRE(results.getCorrected() == sentence.second);
        }
    }

//    SECTION("Test 6.7 Changes") {
//        xstring input =
//        XL("AbstractHowdoesauser’spriorexperiencewithdeeplearningimpactaccuracy?Wepresentaninitialstudybased"
//        "on31participantswithdifferentlevelsofexperience.Theirtaskistoperformhyperparameteroptimizationfor"
//        "agivendeeplearningarchitecture.There-sultsshowastrongpositivecorrelationbetweentheparticipant’sexperience"
//        "andtheﬁnalperformance.Theyadditionallyindicatethatanexperiencedparticipantﬁndsbettersolu-tions"
//        "usingfewerresourcesonaverage.Thedatasuggestsfurthermorethatparticipantswithnopriorexperiencefollow"
//        "randomstrategiesintheirpursuitofoptimalhyperpa-rameters.Ourstudyinvestigatesthesubjectivehumanfactor"
//        "incomparisonsofstateoftheartresultsandscientiﬁcreproducibilityindeeplearning.1IntroductionThepopularity"
//        "ofdeeplearninginvariousﬁeldssuchasimagerecognition[9,19],speech[11,30],bioinformatics[21,24],"
//        "questionanswering[3]etc.stemsfromtheseeminglyfavorabletrade-offbetweentherecognitionaccuracy"
//        "andtheiroptimizationburdenlecunetal20attributetheirsuccess");
//        XL("AbstractHowdoesauser’spriorexperience?");

//        xstring output =
//        XL("Abstract How does a user’s prior experience with deep learning impact accuracy? We present an initial "
//        "study based on 31 participants with different levels of experience. Their task is to perform hyper "
//        "parameter optimization for a given deep learning architecture. The results show a strong positive "
//        "correlation between the participant’s experience and the final performance. They additionally indicate "
//        "that an experienced participant finds better solutions using fewer resources on average. The data "
//        "suggests furthermore that participants with no prior experience follow random strategies in their "
//        "pursuit of optimal hyper parameters. Our study investigates the subjective human factor in comparisons "
//        "of state of the art results and scientific reproducibility in deep learning. 1 Introduction The "
//        "popularity of deep learning in various fields such as image recognition [9,19], speech [11,30], bio "
//        "informatics [21,24], question answering [3] etc. stems from the seemingly favorable trade off between "
//        "the recognition accuracy and their optimization burden l ecu net al 20 attribute their success");
//        XL("Abstract How does a user’s prior experience?");
//
//        SymSpell symSpell(maxEditDistance, prefixLength);
//        symSpell.LoadDictionary("../resources/frequency_dictionary_en_82_765.txt", 0, 1, XL(' '));
//
//        auto results = symSpell.WordSegmentation(input, 2, 28);
//        REQUIRE(results.getSegmented() == output);
//    }

    SECTION("Do Spell Correction") {
        SymSpell symSpell(maxEditDistance, prefixLength);
        symSpell.LoadDictionary("../resources/frequency_dictionary_en_82_765.txt", 0, 1, XL(' '));
        std::unordered_map<xstring, xstring> words = {
                {XL("tke"),          XL("the")},
                {XL("abolution"),    XL("abolition")},
                {XL("intermedaite"), XL("intermediate")}
        };

        for (auto &word : words) {
            auto results = symSpell.Lookup(word.first, Verbosity::Closest);
            REQUIRE(results[0].term == word.second);
        }
    }

    SECTION("Do Spell Correction With MaxEditDistance=2") {
        SymSpell symSpell(maxEditDistance, prefixLength);
        symSpell.LoadDictionary("../resources/frequency_dictionary_en_82_765.txt", 0, 1, XL(' '));
        std::unordered_map<xstring, xstring> words_within_distance = {
                {XL("tke"),     XL("the")},
                {XL("extrine"), XL("extreme")}
        };

        std::unordered_map<xstring, xstring> words_far_distance = {
                {XL("elipnaht"),   XL("elephant")},
                {XL("aotocrasie"), XL("autocracy")}
        };

        for (auto &word : words_within_distance) {
            auto results = symSpell.Lookup(word.first, Verbosity::Closest, 2);
            REQUIRE(results[0].term == word.second);
        }

        for (auto &word : words_far_distance) {
            auto results = symSpell.Lookup(word.first, Verbosity::Closest, 2);
            REQUIRE(results.empty());
        }
    }

    SECTION("Correct Compound Mistakes") {
        SymSpell symSpell(maxEditDistance, prefixLength);
        symSpell.LoadDictionary("../resources/frequency_dictionary_en_82_765.txt", 0, 1, XL(' '));
        symSpell.LoadBigramDictionary("../resources/frequency_bigramdictionary_en_243_342.txt", 0, 2, XL(' '));
        std::unordered_map<xstring, xstring> compunded_sentences = {
                {XL("whereis th elove"),                                                                                  XL("where is the love")},
                {XL("whereis th elove hehad dated forImuch of thepast who couqdn'tread in sixthgrade and ins pired him"), XL("where is the love he had dated for much of the past who couldn't read in sixth grade and inspired him")},
                {XL("in te dhird qarter oflast jear he hadlearned ofca sekretplan"),                                      XL("in the third quarter of last year he had learned of a secret plan")},
                {XL("the bigjest playrs in te strogsommer film slatew ith plety of funn"),                                XL("the biggest players in the strong summer film slate with plenty of fun")}
        };

        for (auto &sentence : compunded_sentences) {
            auto results = symSpell.LookupCompound(sentence.first);
            REQUIRE(results[0].term == sentence.second);
        }
    }

    SECTION("Check top verbosity") {
        SymSpell symSpellcustom(maxEditDistance, prefixLength);
        symSpellcustom.LoadDictionary("../resources/frequency_dictionary_en_test_verbosity.txt", 0, 1, XL(' '));
        auto results = symSpellcustom.Lookup(XL("stream"), Verbosity::Top, 2);
        REQUIRE(1 == results.size());
        REQUIRE(XL("streamc") == results[0].term);
    }

    SECTION("Check all verbosity") {
        SymSpell symSpellcustom(maxEditDistance, prefixLength);
        symSpellcustom.LoadDictionary("../resources/frequency_dictionary_en_test_verbosity.txt", 0, 1, XL(' '));
        auto results = symSpellcustom.Lookup(XL("stream"), Verbosity::All, 2);
        REQUIRE(2 == results.size());
    }

    SECTION("check custom entry of dictionary") {
        SymSpell symSpellcustom(maxEditDistance, prefixLength, DEFAULT_COUNT_THRESHOLD, DEFAULT_INITIAL_CAPACITY,
                                DEFAULT_COMPACT_LEVEL);
        auto staging = std::make_shared<SuggestionStage>(100);
        symSpellcustom.CreateDictionaryEntry(XL("take"), 4, staging);
        auto results = symSpellcustom.Lookup(XL("take"), Verbosity::Closest, 2);
        REQUIRE(XL("take") == results[0].term);
    }

    SECTION("check save works fine.") {
        SymSpell symSpellcustom(maxEditDistance, prefixLength, DEFAULT_COUNT_THRESHOLD, DEFAULT_INITIAL_CAPACITY,
                                DEFAULT_COMPACT_LEVEL);
        symSpellcustom.LoadDictionary("../resources/frequency_dictionary_en_test_verbosity.txt", 0, 1, XL(' '));
        auto filepath = "../resources/model.bin";
        std::ofstream binary_path(filepath, std::ios::out | std::ios::app | std::ios::binary);
        if (binary_path.is_open()) {
            cereal::BinaryOutputArchive oarchive(binary_path);
            oarchive(symSpellcustom);
        } else {
            throw std::invalid_argument("Cannot save to file");
        }
        std::remove(filepath);
    }

    SECTION("Compund mistakes distance") {
        SymSpell symSpell(maxEditDistance, prefixLength);
        symSpell.LoadDictionary("../resources/frequency_dictionary_en_82_765.txt", 0, 1, XL(' '));
        symSpell.LoadBigramDictionary("../resources/frequency_bigramdictionary_en_243_342.txt", 0, 2, XL(' '));
        xstring typo = XL("the bigjest playrs");
        xstring correction = XL("the biggest players");
        auto results = symSpell.LookupCompound(typo, 2);
        REQUIRE(results[0].distance == 2);
    }

    SECTION("Compund mistakes capitals") {
        SymSpell symSpell(maxEditDistance, prefixLength);
        symSpell.LoadDictionary("../resources/frequency_dictionary_en_82_765.txt", 0, 1, XL(' '));
        xstring typo = XL("can yu readthis");
        xstring correction = XL("can you read this");
        auto results = symSpell.LookupCompound(typo, 2);
        REQUIRE(results[0].term == correction);
    }

    SECTION("Lookup transfer casing") {
        SymSpell symSpell(maxEditDistance, prefixLength);
        symSpell.LoadDictionary("../resources/frequency_dictionary_en_82_765.txt", 0, 1, XL(' '));
        xstring typo = XL("meMberSa");
        xstring correction = XL("meMberS");
        auto results = symSpell.Lookup(typo, Verbosity::Top, 2, false, true);
        REQUIRE(results[0].term == correction);
    }
}