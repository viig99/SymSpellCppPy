import unittest
from SymSpellCppPy import SymSpell, Verbosity, SuggestItem
import os
import sys


class SymSpellCppPyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.symSpell = SymSpell()
        cls.symSpell.load_dictionary("resources/frequency_dictionary_en_82_765.txt", 0, 1, " ")
        cls.fortests_path = "tests/fortests"
        cls.dictionary_path = "resources/frequency_dictionary_en_82_765.txt"
        cls.bigram_path = "resources/frequency_bigramdictionary_en_243_342.txt"

    def test_negative_max_dictionary_edit_distance(self):
        self.assertRaisesRegex(ValueError, ".*max_dictionary_edit_distance cannot be negative.*",
                               SymSpell, -1, 3)

    def test_invalid_prefix_length(self):
        self.assertRaisesRegex(ValueError, ".*prefix_length.*", SymSpell, 1, 0)
        self.assertRaisesRegex(ValueError, ".*prefix_length.*", SymSpell, 1, -1)
        self.assertRaisesRegex(ValueError, ".*prefix_length.*", SymSpell, 2, 2)

    def test_negative_count_threshold(self):
        self.assertRaisesRegex(ValueError, ".*count_threshold.*", SymSpell, 1, 3, -1)

    def test_create_dictionary_entry_negative_count(self):
        sym_spell = SymSpell(1, 3)
        self.assertEqual(False, sym_spell.create_dictionary_entry("pipe", 0))
        self.assertEqual(False,
                         sym_spell.create_dictionary_entry("pipe", -1))

        sym_spell = SymSpell(1, 3, count_threshold=0)
        self.assertEqual(True, sym_spell.create_dictionary_entry("pipe", 0))

    def test_deletes(self):
        sym_spell = SymSpell()
        sym_spell.create_dictionary_entry("steama", 4)
        sym_spell.create_dictionary_entry("steamb", 6)
        sym_spell.create_dictionary_entry("steamc", 2)
        result = sym_spell.lookup("stream", Verbosity.TOP, 2)
        self.assertEqual(1, len(result))
        self.assertEqual("steamb", result[0].term)
        self.assertEqual(6, result[0].count)
        self.assertTrue(sym_spell.entry_count())

    def test_words_with_shared_prefix_should_retain_counts(self):
        sym_spell = SymSpell(1, 3)
        sym_spell.create_dictionary_entry("pipe", 5)
        sym_spell.create_dictionary_entry("pips", 10)

        result = sym_spell.lookup("pipe", Verbosity.ALL, 1)
        self.assertEqual(2, len(result))
        self.assertEqual("pipe", result[0].term)
        self.assertEqual(5, result[0].count)
        self.assertEqual("pips", result[1].term)
        self.assertEqual(10, result[1].count)

        result = sym_spell.lookup("pips", Verbosity.ALL, 1)
        self.assertEqual(2, len(result))
        self.assertEqual("pips", result[0].term)
        self.assertEqual(10, result[0].count)
        self.assertEqual("pipe", result[1].term)
        self.assertEqual(5, result[1].count)

        result = sym_spell.lookup("pip", Verbosity.ALL, 1)
        self.assertEqual(2, len(result))
        self.assertEqual("pips", result[0].term)
        self.assertEqual(10, result[0].count)
        self.assertEqual("pipe", result[1].term)
        self.assertEqual(5, result[1].count)

    def test_add_additional_counts_should_not_add_word_again(self):
        sym_spell = SymSpell()
        word = "hello"
        sym_spell.create_dictionary_entry(word, 11)
        self.assertEqual(1, sym_spell.word_count())

        sym_spell.create_dictionary_entry(word, 3)
        self.assertEqual(1, sym_spell.word_count())

    def test_add_additional_counts_should_increase_count(self):
        sym_spell = SymSpell()
        word = "hello"
        sym_spell.create_dictionary_entry(word, 11)
        result = sym_spell.lookup(word, Verbosity.ALL)
        count = result[0].count if len(result) == 1 else 0
        self.assertEqual(11, count)

        sym_spell.create_dictionary_entry(word, 3)
        result = sym_spell.lookup(word, Verbosity.ALL)
        count = result[0].count if len(result) == 1 else 0
        self.assertEqual(11 + 3, count)

    def test_add_additional_counts_should_not_overflow(self):
        sym_spell = SymSpell()
        word = "hello"
        sym_spell.create_dictionary_entry(word, sys.maxsize - 10)
        result = sym_spell.lookup(word, Verbosity.ALL)
        count = result[0].count if len(result) == 1 else 0
        self.assertEqual(sys.maxsize - 10, count)

        sym_spell.create_dictionary_entry(word, 11)
        result = sym_spell.lookup(word, Verbosity.ALL)
        count = result[0].count if len(result) == 1 else 0
        self.assertEqual(sys.maxsize, count)

    def test_verbosity_should_control_lookup_results(self):
        sym_spell = SymSpell()
        sym_spell.create_dictionary_entry("steam", 1)
        sym_spell.create_dictionary_entry("steams", 2)
        sym_spell.create_dictionary_entry("steem", 3)

        result = sym_spell.lookup("steems", Verbosity.TOP, 2)
        self.assertEqual(1, len(result))
        result = sym_spell.lookup("steems", Verbosity.CLOSEST, 2)
        self.assertEqual(2, len(result))
        result = sym_spell.lookup("steems", Verbosity.ALL, 2)
        self.assertEqual(3, len(result))

    def test_lookup_should_return_most_frequent(self):
        sym_spell = SymSpell()
        sym_spell.create_dictionary_entry("steama", 4)
        sym_spell.create_dictionary_entry("steamb", 6)
        sym_spell.create_dictionary_entry("steamc", 2)
        result = sym_spell.lookup("stream", Verbosity.TOP, 2)
        self.assertEqual(1, len(result))
        self.assertEqual("steamb", result[0].term)
        self.assertEqual(6, result[0].count)

    def test_lookup_should_find_exact_match(self):
        sym_spell = SymSpell()
        sym_spell.create_dictionary_entry("steama", 4)
        sym_spell.create_dictionary_entry("steamb", 6)
        sym_spell.create_dictionary_entry("steamc", 2)
        result = sym_spell.lookup("streama", Verbosity.TOP, 2)
        self.assertEqual(1, len(result))
        self.assertEqual("steama", result[0].term)

    def test_lookup_should_not_return_non_word_delete(self):
        sym_spell = SymSpell(2, 7, 10)
        sym_spell.create_dictionary_entry("pawn", 10)
        result = sym_spell.lookup("paw", Verbosity.TOP, 0)
        self.assertEqual(0, len(result))
        result = sym_spell.lookup("awn", Verbosity.TOP, 0)
        self.assertEqual(0, len(result))

    def test_lookup_should_not_return_low_count_word(self):
        sym_spell = SymSpell(2, 7, 10)
        sym_spell.create_dictionary_entry("pawn", 1)
        result = sym_spell.lookup("pawn", Verbosity.TOP, 0)
        self.assertEqual(0, len(result))

    def test_lookup_should_not_return_low_count_word_that_are_also_delete_word(self):
        sym_spell = SymSpell(2, 7, 10)
        sym_spell.create_dictionary_entry("flame", 20)
        sym_spell.create_dictionary_entry("flam", 1)
        result = sym_spell.lookup("flam", Verbosity.TOP, 0)
        self.assertEqual(0, len(result))

    def test_lookup_max_edit_distance_too_large(self):
        sym_spell = SymSpell(2, 7, 10)
        sym_spell.create_dictionary_entry("flame", 20)
        sym_spell.create_dictionary_entry("flam", 1)
        self.assertRaisesRegex(ValueError, ".*Distance too large.*", sym_spell.lookup, "flam",
                               Verbosity.TOP, 3)

    def test_lookup_include_unknown(self):
        sym_spell = SymSpell(2, 7, 10)
        sym_spell.create_dictionary_entry("flame", 20)
        sym_spell.create_dictionary_entry("flam", 1)
        result = sym_spell.lookup("flam", Verbosity.TOP, 0, True)
        self.assertEqual(1, len(result))
        self.assertEqual("flam", result[0].term)

    def test_load_bigram_dictionary_invalid_path(self):
        edit_distance_max = 2
        prefix_length = 7
        sym_spell = SymSpell(edit_distance_max, prefix_length)
        self.assertEqual(False, sym_spell.load_bigram_dictionary(
            "invalid/dictionary/path.txt", 0, 2))

    def test_loading_dictionary_from_fileobject(self):
        big_words_path = os.path.join(self.fortests_path, "big_words.txt")
        edit_distance_max = 2
        prefix_length = 7
        sym_spell = SymSpell(edit_distance_max, prefix_length)
        self.assertEqual(True, sym_spell.create_dictionary(big_words_path))

    def test_load_bigram_dictionary_bad_dict(self):
        dictionary_path = os.path.join(self.fortests_path,
                                       "bad_dict.txt")
        edit_distance_max = 2
        prefix_length = 7
        sym_spell = SymSpell(edit_distance_max, prefix_length)
        self.assertEqual(True, sym_spell.load_bigram_dictionary(
            dictionary_path, 0, 2))

    def test_load_bigram_dictionary_separator(self):
        dictionary_path = os.path.join(self.fortests_path,
                                       "separator_dict.txt")
        edit_distance_max = 2
        prefix_length = 7
        sym_spell = SymSpell(edit_distance_max, prefix_length)
        self.assertEqual(True, sym_spell.load_bigram_dictionary(
            dictionary_path, 0, 1, "$"))

    def test_load_dictionary_invalid_path(self):
        edit_distance_max = 2
        prefix_length = 7
        sym_spell = SymSpell(edit_distance_max, prefix_length)
        self.assertEqual(False, sym_spell.load_dictionary(
            "invalid/dictionary/path.txt", 0, 1))

    def test_load_dictionary_bad_dictionary(self):
        dictionary_path = os.path.join(self.fortests_path, "bad_dict.txt")
        edit_distance_max = 2
        prefix_length = 7
        sym_spell = SymSpell(edit_distance_max, prefix_length)
        self.assertEqual(True, sym_spell.load_dictionary(
            dictionary_path, 0, 1))
        self.assertEqual(7, sym_spell.word_count())

    def test_load_dictionary_separator(self):
        dictionary_path = os.path.join(self.fortests_path,
                                       "separator_dict.txt")
        edit_distance_max = 2
        prefix_length = 7
        sym_spell = SymSpell(edit_distance_max, prefix_length)
        self.assertEqual(True, sym_spell.load_dictionary(
            dictionary_path, 0, 1, "$"))
        self.assertEqual(5, sym_spell.word_count())

    def test_lookup_should_replicate_noisy_results(self):
        query_path = os.path.join(self.fortests_path,
                                  "noisy_query_en_1000.txt")

        edit_distance_max = 2
        prefix_length = 7
        verbosity = Verbosity.CLOSEST

        test_list = []
        with open(query_path, "r") as infile:
            for line in infile.readlines():
                line_parts = line.rstrip().split(" ")
                if len(line_parts) >= 2:
                    test_list.append(line_parts[0])
        result_sum = 0
        for phrase in test_list:
            result_sum += len(self.symSpell.lookup(phrase, verbosity,
                                                   edit_distance_max))
        self.assertEqual(4945, result_sum)

    def test_lookup_compound(self):
        edit_distance_max = 2
        prefix_length = 7
        sym_spell = SymSpell(edit_distance_max, prefix_length)
        sym_spell.load_dictionary(self.dictionary_path, 0, 1)
        sym_spell.load_bigram_dictionary(self.bigram_path, 0, 2)

        typo = "whereis th elove"
        correction = "where is the love"
        results = sym_spell.lookup_compound(typo, edit_distance_max)
        self.assertEqual(1, len(results))
        self.assertEqual(correction, results[0].term)
        self.assertEqual(2, results[0].distance)
        self.assertEqual(585, results[0].count)

        typo = "the bigjest playrs"
        correction = "the biggest players"
        results = sym_spell.lookup_compound(typo, edit_distance_max)
        self.assertEqual(1, len(results))
        self.assertEqual(correction, results[0].term)
        self.assertEqual(2, results[0].distance)
        self.assertEqual(34, results[0].count)

        typo = "can yu readthis"
        correction = "can you read this"
        results = sym_spell.lookup_compound(typo, edit_distance_max)
        self.assertEqual(1, len(results))
        self.assertEqual(correction, results[0].term)
        self.assertEqual(2, results[0].distance)
        self.assertEqual(11440, results[0].count)

        typo = ("whereis th elove hehad dated forImuch of thepast who "
                "couqdn'tread in sixthgrade and ins pired him")
        correction = ("where is the love he had dated for much of the past "
                      "who couldn't read in sixth grade and inspired him")
        results = sym_spell.lookup_compound(typo, edit_distance_max)
        self.assertEqual(1, len(results))
        self.assertEqual(correction, results[0].term)
        self.assertEqual(9, results[0].distance)
        self.assertEqual(0, results[0].count)

        typo = "in te dhird qarter oflast jear he hadlearned ofca sekretplan"
        correction = ("in the third quarter of last year he had learned of a "
                      "secret plan")
        results = sym_spell.lookup_compound(typo, edit_distance_max)
        self.assertEqual(1, len(results))
        self.assertEqual(correction, results[0].term)
        self.assertEqual(9, results[0].distance)
        self.assertEqual(0, results[0].count)

        typo = ("the bigjest playrs in te strogsommer film slatew ith plety "
                "of funn")
        correction = ("the biggest players in the strong summer film slate "
                      "with plenty of fun")
        results = sym_spell.lookup_compound(typo, edit_distance_max)
        self.assertEqual(1, len(results))
        self.assertEqual(correction, results[0].term)
        self.assertEqual(9, results[0].distance)
        self.assertEqual(0, results[0].count)

        typo = ("can yu readthis messa ge despite thehorible sppelingmsitakes")
        correction = ("can you read this message despite the horrible "
                      "spelling mistakes")
        results = sym_spell.lookup_compound(typo, edit_distance_max)
        self.assertEqual(1, len(results))
        self.assertEqual(correction, results[0].term)
        self.assertEqual(9, results[0].distance)
        self.assertEqual(0, results[0].count)

    def test_lookup_compound_no_bigram(self):
        edit_distance_max = 2
        prefix_length = 7
        sym_spell = SymSpell(edit_distance_max, prefix_length)
        sym_spell.load_dictionary(self.dictionary_path, 0, 1)

        typo = "whereis th elove"
        correction = "whereas the love"
        results = sym_spell.lookup_compound(typo, edit_distance_max)
        self.assertEqual(1, len(results))
        self.assertEqual(correction, results[0].term)
        self.assertEqual(2, results[0].distance)
        self.assertEqual(64, results[0].count)

        typo = "the bigjest playrs"
        correction = "the biggest players"
        results = sym_spell.lookup_compound(typo, edit_distance_max)
        self.assertEqual(1, len(results))
        self.assertEqual(correction, results[0].term)
        self.assertEqual(2, results[0].distance)
        self.assertEqual(34, results[0].count)

        typo = "can yu readthis"
        correction = "can you read this"
        results = sym_spell.lookup_compound(typo, edit_distance_max)
        self.assertEqual(1, len(results))
        self.assertEqual(correction, results[0].term)
        self.assertEqual(2, results[0].distance)
        self.assertEqual(3, results[0].count)

        typo = ("whereis th elove hehad dated forImuch of thepast who "
                "couqdn'tread in sixthgrade and ins pired him")
        correction = ("whereas the love head dated for much of the past who "
                      "couldn't read in sixth grade and inspired him")
        results = sym_spell.lookup_compound(typo, edit_distance_max)
        self.assertEqual(1, len(results))
        self.assertEqual(correction, results[0].term)
        self.assertEqual(9, results[0].distance)
        self.assertEqual(0, results[0].count)

        typo = "in te dhird qarter oflast jear he hadlearned ofca sekretplan"
        correction = ("in the third quarter of last year he had learned of "
                      "a secret plan")
        results = sym_spell.lookup_compound(typo, edit_distance_max)
        self.assertEqual(1, len(results))
        self.assertEqual(correction, results[0].term)
        self.assertEqual(9, results[0].distance)
        self.assertEqual(0, results[0].count)

        typo = ("the bigjest playrs in te strogsommer film slatew ith plety "
                "of funn")
        correction = ("the biggest players in the strong summer film slate "
                      "with plenty of fun")
        results = sym_spell.lookup_compound(typo, edit_distance_max)
        self.assertEqual(1, len(results))
        self.assertEqual(correction, results[0].term)
        self.assertEqual(9, results[0].distance)
        self.assertEqual(0, results[0].count)

        typo = ("can yu readthis messa ge despite thehorible sppelingmsitakes")
        correction = ("can you read this message despite the horrible "
                      "spelling mistakes")
        results = sym_spell.lookup_compound(typo, edit_distance_max)
        self.assertEqual(1, len(results))
        self.assertEqual(correction, results[0].term)
        self.assertEqual(9, results[0].distance)
        self.assertEqual(0, results[0].count)

    def test_lookup_compound_only_combi(self):
        edit_distance_max = 2
        prefix_length = 7
        sym_spell = SymSpell(edit_distance_max, prefix_length)
        sym_spell.create_dictionary_entry("steam", 1)
        sym_spell.create_dictionary_entry("machine", 1)

        typo = "ste am machie"
        correction = "steam machine"
        results = sym_spell.lookup_compound(typo, edit_distance_max)
        self.assertEqual(1, len(results))
        self.assertEqual(correction, results[0].term)

    def test_lookup_compound_no_suggestion(self):
        edit_distance_max = 2
        prefix_length = 7
        sym_spell = SymSpell(edit_distance_max, prefix_length)
        sym_spell.create_dictionary_entry("steam", 1)
        sym_spell.create_dictionary_entry("machine", 1)

        typo = "qwer erty ytui a"
        results = sym_spell.lookup_compound(typo, edit_distance_max)
        self.assertEqual(1, len(results))
        self.assertEqual(typo, results[0].term)

    def test_load_dictionary_encoding(self):
        dictionary_path = os.path.join(self.fortests_path, "non_en_dict.txt")

        edit_distance_max = 2
        prefix_length = 7
        sym_spell = SymSpell(edit_distance_max, prefix_length)
        sym_spell.load_dictionary(dictionary_path, 0, 1)

        result = sym_spell.lookup("АБ", Verbosity.TOP, 2)
        self.assertEqual(1, len(result))
        self.assertEqual("АБИ", result[0].term)

    def test_word_segmentation(self):
        edit_distance_max = 0
        prefix_length = 7
        sym_spell = SymSpell(edit_distance_max, prefix_length)
        sym_spell.load_dictionary(self.dictionary_path, 0, 1)

        typo = "thequickbrownfoxjumpsoverthelazydog"
        correction = "the quick brown fox jumps over the lazy dog"
        result = sym_spell.word_segmentation(typo)
        self.assertEqual(correction, result.corrected_string)

        typo = "itwasabrightcolddayinaprilandtheclockswerestrikingthirteen"
        correction = ("it was a bright cold day in april and the clocks "
                      "were striking thirteen")
        result = sym_spell.word_segmentation(typo)
        self.assertEqual(correction, result.segmented_string)

        typo = ("itwasthebestoftimesitwastheworstoftimesitwastheageofwisdom"
                "itwastheageoffoolishness")
        correction = ("it was the best of times it was the worst of times "
                      "it was the age of wisdom it was the age of foolishness")
        result = sym_spell.word_segmentation(typo)
        self.assertEqual(correction, result.segmented_string)

    def test_word_segmentation_with_arguments(self):
        edit_distance_max = 0
        prefix_length = 7
        sym_spell = SymSpell(edit_distance_max, prefix_length)
        sym_spell.load_dictionary(self.dictionary_path, 0, 1)

        typo = "thequickbrownfoxjumpsoverthelazydog"
        correction = "the quick brown fox jumps over the lazy dog"
        result = sym_spell.word_segmentation(typo, edit_distance_max, 11)
        self.assertEqual(correction, result.corrected_string)

        typo = "itwasabrightcolddayinaprilandtheclockswerestrikingthirteen"
        correction = ("it was a bright cold day in april and the clocks "
                      "were striking thirteen")
        result = sym_spell.word_segmentation(typo, edit_distance_max, 11)
        self.assertEqual(correction, result.corrected_string)

        typo = (" itwasthebestoftimesitwastheworstoftimesitwastheageofwisdom"
                "itwastheageoffoolishness")
        correction = ("it was the best of times it was the worst of times "
                      "it was the age of wisdom it was the age of foolishness")
        result = sym_spell.word_segmentation(typo, edit_distance_max, 11)
        self.assertEqual(correction, result.corrected_string)

    def test_word_segmentation_capitalize(self):
        edit_distance_max = 0
        prefix_length = 7
        sym_spell = SymSpell(edit_distance_max, prefix_length)
        sym_spell.load_dictionary(self.dictionary_path, 0, 1)

        typo = "Thequickbrownfoxjumpsoverthelazydog"
        correction = "The quick brown fox jumps over the lazy dog"
        result = sym_spell.word_segmentation(typo)
        self.assertEqual(correction, result.corrected_string)

        typo = "Itwasabrightcolddayinaprilandtheclockswerestrikingthirteen"
        correction = ("It was a bright cold day in april and the clocks "
                      "were striking thirteen")
        result = sym_spell.word_segmentation(typo)
        self.assertEqual(correction, result.segmented_string)

        typo = ("Itwasthebestoftimesitwastheworstoftimesitwastheageofwisdom"
                "itwastheageoffoolishness")
        correction = ("It was the best of times it was the worst of times "
                      "it was the age of wisdom it was the age of foolishness")
        result = sym_spell.word_segmentation(typo)
        self.assertEqual(correction, result.segmented_string)

    def test_word_segmentation_apostrophe(self):
        edit_distance_max = 0
        prefix_length = 7
        sym_spell = SymSpell(edit_distance_max, prefix_length)
        sym_spell.load_dictionary(self.dictionary_path, 0, 1)

        typo = "There'resomewords"
        correction = ("There' re some words")
        result = sym_spell.word_segmentation(typo)
        self.assertEqual(correction, result.corrected_string)

    def test_suggest_item(self):
        si_1 = SuggestItem("asdf", 12, 34)
        si_2 = SuggestItem("sdfg", 12, 34)
        si_3 = SuggestItem("dfgh", 56, 78)

        self.assertTrue(si_1 == si_1)
        self.assertFalse(si_2 == si_3)

        self.assertEqual("asdf", si_1.term)
        si_1.term = "qwer"
        self.assertEqual("qwer", si_1.term)

        self.assertEqual(34, si_1.count)
        si_1.count = 78
        self.assertEqual(78, si_1.count)

        self.assertEqual("qwer, 12, 78", str(si_1))

    def test_create_dictionary_invalid_path(self):
        edit_distance_max = 2
        prefix_length = 7
        sym_spell = SymSpell(edit_distance_max, prefix_length)
        self.assertEqual(False, sym_spell.create_dictionary(
            "invalid/dictionary/path.txt"))

    def test_create_dictionary(self):
        corpus_path = os.path.join(self.fortests_path, "big_modified.txt")
        big_words_path = os.path.join(self.fortests_path, "big_words.txt")

        edit_distance_max = 2
        prefix_length = 7
        sym_spell = SymSpell(edit_distance_max, prefix_length)
        sym_spell.create_dictionary(corpus_path)
        self.assertEqual(68, sym_spell.max_length())

    def test_pickle_compressed(self):
        pickle_path = os.path.join(self.fortests_path, "dictionary.pickle")
        edit_distance_max = 2
        prefix_length = 7
        sym_spell = SymSpell(edit_distance_max, prefix_length)
        sym_spell.load_dictionary(self.dictionary_path, 0, 1)
        sym_spell.save_pickle(pickle_path)

        sym_spell_2 = SymSpell(edit_distance_max, prefix_length)
        sym_spell_2.load_pickle(pickle_path)
        self.assertEqual(sym_spell.max_length(), sym_spell_2.max_length())
        self.assertEqual(sym_spell.lookup("flam", Verbosity.TOP, 0, True)[0].term,
                         sym_spell_2.lookup("flam", Verbosity.TOP, 0, True)[0].term)
        os.remove(pickle_path)

    def test_delete_dictionary_entry(self):
        sym_spell = SymSpell()
        sym_spell.create_dictionary_entry("stea", 1)
        sym_spell.create_dictionary_entry("steama", 2)
        sym_spell.create_dictionary_entry("steem", 3)

        result = sym_spell.lookup("steama", Verbosity.TOP, 2)
        self.assertEqual(1, len(result))
        self.assertEqual("steama", result[0].term)
        self.assertEqual(len("steama"), sym_spell.max_length())
        self.assertEqual(3, sym_spell.word_count())

        self.assertTrue(sym_spell.delete_dictionary_entry("steama"))
        self.assertEqual(len("steem"), sym_spell.max_length())
        self.assertEqual(2, sym_spell.word_count())
        result = sym_spell.lookup("steama", Verbosity.TOP, 2)
        self.assertEqual(1, len(result))
        self.assertEqual("steem", result[0].term)

        self.assertTrue(sym_spell.delete_dictionary_entry("stea"))
        self.assertEqual(len("steem"), sym_spell.max_length())
        self.assertEqual(1, sym_spell.word_count())
        result = sym_spell.lookup("steama", Verbosity.TOP, 2)
        self.assertEqual(1, len(result))
        self.assertEqual("steem", result[0].term)

    def test_delete_dictionary_entry_invalid_word(self):
        sym_spell = SymSpell()
        sym_spell.create_dictionary_entry("stea", 1)
        sym_spell.create_dictionary_entry("steama", 2)
        sym_spell.create_dictionary_entry("steem", 3)

        result = sym_spell.lookup("steama", Verbosity.TOP, 2)
        self.assertEqual(1, len(result))
        self.assertEqual("steama", result[0].term)
        self.assertEqual(len("steama"), sym_spell.max_length())

        self.assertFalse(sym_spell.delete_dictionary_entry("steamab"))
        result = sym_spell.lookup("steama", Verbosity.TOP, 2)
        self.assertEqual(1, len(result))
        self.assertEqual("steama", result[0].term)
        self.assertEqual(len("steama"), sym_spell.max_length())

    def test_lookup_compound_transfer_casing(self):
        edit_distance_max = 2
        prefix_length = 7
        sym_spell = SymSpell(edit_distance_max, prefix_length)
        sym_spell.load_dictionary(self.dictionary_path, 0, 1)
        sym_spell.load_bigram_dictionary(self.bigram_path, 0, 2)

        typo = ("Whereis th elove hehaD Dated forImuch of thepast who "
                "couqdn'tread in sixthgrade AND ins pired him")
        correction = ("Where is the love he haD Dated for much of the past "
                      "who couldn't read in sixth grade AND inspired him")

        results = sym_spell.lookup_compound(typo, edit_distance_max,
                                            transfer_casing=True)
        self.assertEqual(correction, results[0].term)

    def test_lookup_compound_transfer_casing_no_bigram(self):
        edit_distance_max = 2
        prefix_length = 7
        sym_spell = SymSpell(edit_distance_max, prefix_length)
        sym_spell.load_dictionary(self.dictionary_path, 0, 1)

        typo = ("Whereis th elove hehaD Dated forImuch of thepast who "
                "couqdn'tread in sixthgrade AND ins pired him")
        correction = ("Whereas the love heaD Dated for much of the past "
                      "who couldn't read in sixth grade AND inspired him")

        results = sym_spell.lookup_compound(typo, edit_distance_max,
                                            transfer_casing=True)
        self.assertEqual(correction, results[0].term)

    # TODO: test_create_dictionary_entry_below_threshold
    # TODO: test_lookup_avoid_exact_match_early_exit
    # TODO: test_lookup_compound_replaced_words
    # TODO: test_lookup_compound_replaced_words_no_bigram
    # TODO: test_lookup_compound_ignore_non_words
    # TODO: test_lookup_compound_ignore_non_words_no_bigram
    # TODO: test_word_segmentation_ignore_token
    # TODO: test_word_segmentation_ligature
    # TODO: test_lookup_compound_transfer_casing_ignore_nonwords
    # TODO: test_lookup_compound_transfer_casing_ignore_nonwords_no_bigram

    def test_lookup(self):
        self.assertEqual(self.symSpell.lookup("tke", Verbosity.CLOSEST)[0].term, "the")
        self.assertEqual(self.symSpell.lookup("abolution", Verbosity.CLOSEST)[0].term, "abolition")
        self.assertEqual(self.symSpell.lookup("intermedaite", Verbosity.CLOSEST)[0].term, "intermediate")

    def test_lookup_distance(self):
        self.assertEqual(self.symSpell.lookup("extrine", Verbosity.CLOSEST, 2)[0].term, "extreme")
        self.assertListEqual(self.symSpell.lookup("extrine", Verbosity.CLOSEST, 1), [])
        self.assertListEqual(self.symSpell.lookup("elipnaht", Verbosity.CLOSEST), [])
        self.assertListEqual(self.symSpell.lookup("aotocrasie", Verbosity.CLOSEST), [])

    def test_compound_mistakes(self):
        self.assertEqual(self.symSpell.lookup_compound(
            "whereis th elove hehad dated forImuch of thepast who couqdn'tread in sixthgrade and ins pired him")[
                             0].term,
                         "whereas the love head dated for much of the past who couldn't read in sixth grade and inspired "
                         "him")
        self.assertEqual(self.symSpell.lookup_compound(
            "in te dhird qarter oflast jear he hadlearned ofca sekretplan")[0].term,
                         "in the third quarter of last year he had learned of a secret plan")
        self.assertEqual(self.symSpell.lookup_compound(
            "the bigjest playrs in te strogsommer film slatew ith plety of funn")[0].term,
                         "the biggest players in the strong summer film slate with plenty of fun")

    def test_word_segementation(self):
        self.assertEqual(self.symSpell.word_segmentation(
            "thequickbrownfoxjumpsoverthelazydog").corrected_string,
                         "the quick brown fox jumps over the lazy dog")
        self.assertEqual(self.symSpell.word_segmentation(
            "itwasabrightcolddayinaprilandtheclockswerestrikingthirteen").corrected_string,
                         "it was bright holiday in april and the clocks were striking thirteen")
        self.assertEqual(self.symSpell.word_segmentation(
            "itwasthebestoftimesitwastheworstoftimesitwastheageofwisdomitwastheageoffoolishness").corrected_string,
                         "iowa the best of times it waste worst of times it was thereof wisdom it was thereof "
                         "foolishness")

    def test_save_load(self):
        before_save = self.symSpell.lookup("tke", Verbosity.CLOSEST)[0].term
        before_max_length = self.symSpell.max_length()
        os.makedirs("temp", exist_ok=True)
        self.symSpell.save_pickle("temp/temp.bin")
        load_sym_spell = SymSpell()
        load_sym_spell.load_pickle("temp/temp.bin")
        after_load = load_sym_spell.lookup("tke", Verbosity.CLOSEST)[0].term
        after_max_length = load_sym_spell.max_length()
        os.remove("temp/temp.bin")
        os.rmdir("temp")
        assert (before_save == after_load)
        assert (before_max_length == after_max_length)

    def test_lookup_transfer_casing(self):
        sym_spell = SymSpell()
        sym_spell.create_dictionary_entry("steam", 4)
        result = sym_spell.lookup("Stream", Verbosity.TOP, 2,
                                  transfer_casing=True)
        self.assertEqual("Steam", result[0].term)

        sym_spell = SymSpell()
        sym_spell.create_dictionary_entry("steam", 4)
        result = sym_spell.lookup("StreaM", Verbosity.TOP, 2,
                                  transfer_casing=True)
        self.assertEqual("SteaM", result[0].term)

        sym_spell = SymSpell()
        sym_spell.create_dictionary_entry("steam", 4)
        result = sym_spell.lookup("STREAM", Verbosity.TOP, 2,
                                  transfer_casing=True)
        self.assertEqual("STEAM", result[0].term)

        sym_spell = SymSpell()
        sym_spell.create_dictionary_entry("i", 4)
        result = sym_spell.lookup("I", Verbosity.TOP, 2,
                                  transfer_casing=True)
        self.assertEqual("I", result[0].term)


if __name__ == '__main__':
    unittest.main()
