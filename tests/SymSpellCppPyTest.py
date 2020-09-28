import unittest
import SymSpellCppPy


class SymSpellCppPyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.symSpell = SymSpellCppPy.SymSpell()
        cls.symSpell.load_dictionary("resources/frequency_dictionary_en_82_765.txt", 0, 1, " ")

    def test_lookup(self):
        self.assertEqual(self.symSpell.lookup_term("tke", SymSpellCppPy.Verbosity.Closest)[0], "take")
        self.assertEqual(self.symSpell.lookup_term("abolution", SymSpellCppPy.Verbosity.Closest)[0], "abolition")
        self.assertEqual(self.symSpell.lookup_term("intermedaite", SymSpellCppPy.Verbosity.Closest)[0], "intermediate")

    def test_lookup_distance(self):
        self.assertEqual(self.symSpell.lookup_term("extrine", SymSpellCppPy.Verbosity.Closest, 2)[0], "extreme")
        self.assertListEqual(self.symSpell.lookup_term("extrine", SymSpellCppPy.Verbosity.Closest, 1), [])
        self.assertListEqual(self.symSpell.lookup_term("elipnaht", SymSpellCppPy.Verbosity.Closest), [])
        self.assertListEqual(self.symSpell.lookup_term("aotocrasie", SymSpellCppPy.Verbosity.Closest), [])

    def test_compound_mistakes(self):
        self.assertEqual(self.symSpell.lookup_compound_term(
            "whereis th elove hehad dated forImuch of thepast who couqdn'tread in sixthgrade and ins pired him")[0],
                         "whereas to love head dated for much of theist who couldn't read in sixth grade and inspired "
                         "him")
        self.assertEqual(self.symSpell.lookup_compound_term(
            "in te dhird qarter oflast jear he hadlearned ofca sekretplan")[0],
                         "in to third quarter of last year he had learned of a secret plan")
        self.assertEqual(self.symSpell.lookup_compound_term(
            "the bigjest playrs in te strogsommer film slatew ith plety of funn")[0],
                         "they biggest players in to strong summer film slate with plenty of fun")

    def test_word_segementation(self):
        self.assertEqual(self.symSpell.word_segmentation(
            "thequickbrownfoxjumpsoverthelazydog").get_corrected(),
                         "they quick brown fox jumps over therapy dog")
        self.assertEqual(self.symSpell.word_segmentation(
            "itwasabrightcolddayinaprilandtheclockswerestrikingthirteen").get_corrected(),
                         "it was bright holiday in april another clocks were striking thirteen")
        self.assertEqual(self.symSpell.word_segmentation(
            "itwasthebestoftimesitwastheworstoftimesitwastheageofwisdomitwastheageoffoolishness").get_corrected(),
                         "it waste best of times it waste worst of times it was thereof wisdom it was thereof "
                         "foolishness")


if __name__ == '__main__':
    unittest.main()
