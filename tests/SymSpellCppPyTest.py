import unittest
import SymSpellCppPy


class SymSpellCppPyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.symSpell = SymSpellCppPy.SymSpell()
        cls.symSpell.LoadDictionary("resources/frequency_dictionary_en_82_765.txt", 0, 1, " ")

    def test_lookup(self):
        self.assertEqual(self.symSpell.LookupTerm("tke", SymSpellCppPy.Verbosity.Closest)[0], "take")
        self.assertEqual(self.symSpell.LookupTerm("abolution", SymSpellCppPy.Verbosity.Closest)[0], "abolition")
        self.assertEqual(self.symSpell.LookupTerm("intermedaite", SymSpellCppPy.Verbosity.Closest)[0], "intermediate")

    def test_lookup_distance(self):
        self.assertEqual(self.symSpell.LookupTerm("extrine", SymSpellCppPy.Verbosity.Closest, 2)[0], "extreme")
        self.assertListEqual(self.symSpell.LookupTerm("extrine", SymSpellCppPy.Verbosity.Closest, 1), [])
        self.assertListEqual(self.symSpell.LookupTerm("elipnaht", SymSpellCppPy.Verbosity.Closest), [])
        self.assertListEqual(self.symSpell.LookupTerm("aotocrasie", SymSpellCppPy.Verbosity.Closest), [])

    def test_compound_mistakes(self):
        self.assertEqual(self.symSpell.LookupCompoundTerm(
            "whereis th elove hehad dated forImuch of thepast who couqdn'tread in sixthgrade and ins pired him")[0],
            "whereas to love head dated for much of theist who couldn't read in sixth grade and inspired him")
        self.assertEqual(self.symSpell.LookupCompoundTerm(
            "in te dhird qarter oflast jear he hadlearned ofca sekretplan")[0],
            "in to third quarter of last year he had learned of a secret plan")
        self.assertEqual(self.symSpell.LookupCompoundTerm(
            "the bigjest playrs in te strogsommer film slatew ith plety of funn")[0],
            "they biggest players in to strong summer film slate with plenty of fun")


if __name__ == '__main__':
    unittest.main()
