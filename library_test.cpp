#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "library.cpp"


TEST_CASE( "Testing English word correction", "[word_correction]" ) {
	const int initialCapacity = 82765;
	const int maxEditDistance = 2;
	const int prefixLength = 3;
	SymSpell symSpellobj(initialCapacity, maxEditDistance, prefixLength);
	symSpellobj.LoadDictionary("resources\\frequency_dictionary_en_82_765.txt", 0, 1,'|');
    std::string teststr = "abolution";
    std::vector<SuggestItem> results = symSpellobj.Lookup(teststr,Verbosity::Closest);
	std::cout << results[0].term;
    REQUIRE(results[0].term == "absolution");
}
