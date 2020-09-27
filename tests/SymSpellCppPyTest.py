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
        self.assertListEqual(self.symSpell.LookupTerm("elipnaht", SymSpellCppPy.Verbosity.Closest), [])
        self.assertListEqual(self.symSpell.LookupTerm("aotocrasie", SymSpellCppPy.Verbosity.Closest), [])


if __name__ == '__main__':
    unittest.main()
