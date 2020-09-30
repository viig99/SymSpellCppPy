import unittest
import SymSpellCppPy
import os


class SymSpellCppPyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.symSpell = SymSpellCppPy.SymSpell()
        cls.symSpell.load_dictionary("resources/frequency_dictionary_en_82_765.txt", 0, 1, " ")

    def test_lookup(self):
        self.assertEqual(self.symSpell.lookup("tke", SymSpellCppPy.Verbosity.CLOSEST)[0].term, "the")
        self.assertEqual(self.symSpell.lookup("abolution", SymSpellCppPy.Verbosity.CLOSEST)[0].term, "abolition")
        self.assertEqual(self.symSpell.lookup("intermedaite", SymSpellCppPy.Verbosity.CLOSEST)[0].term, "intermediate")

    def test_lookup_distance(self):
        self.assertEqual(self.symSpell.lookup("extrine", SymSpellCppPy.Verbosity.CLOSEST, 2)[0].term, "extreme")
        self.assertListEqual(self.symSpell.lookup("extrine", SymSpellCppPy.Verbosity.CLOSEST, 1), [])
        self.assertListEqual(self.symSpell.lookup("elipnaht", SymSpellCppPy.Verbosity.CLOSEST), [])
        self.assertListEqual(self.symSpell.lookup("aotocrasie", SymSpellCppPy.Verbosity.CLOSEST), [])

    def test_compound_mistakes(self):
        self.assertEqual(self.symSpell.lookup_compound(
            "whereis th elove hehad dated forImuch of thepast who couqdn'tread in sixthgrade and ins pired him")[0].term,
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
        before_save = self.symSpell.lookup("tke", SymSpellCppPy.Verbosity.CLOSEST)[0].term
        before_max_length = self.symSpell.max_length()
        os.makedirs("temp", exist_ok=True)
        self.symSpell.save_pickle("temp/temp.bin")
        load_sym_spell = SymSpellCppPy.SymSpell()
        load_sym_spell.load_pickle("temp/temp.bin")
        after_load = load_sym_spell.lookup("tke", SymSpellCppPy.Verbosity.CLOSEST)[0].term
        after_max_length = load_sym_spell.max_length()
        os.remove("temp/temp.bin")
        os.rmdir("temp")
        assert(before_save == after_load)
        assert(before_max_length == after_max_length)


if __name__ == '__main__':
    unittest.main()
