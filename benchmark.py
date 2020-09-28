'''
    To Check install
    pip install pytest pytest-benchmark symspellpy SymSpellCppPy
    pytest benchmark.py
'''

from symspellpy import SymSpell, Verbosity
from SymSpellCppPy import SymSpell as SymSpellCpp, Verbosity as VerbosityCpp
import time

dict_path = "resources/frequency_dictionary_en_82_765.txt"

def test_load_dict_symspellpy(benchmark):
    sym_spell = SymSpell(max_dictionary_edit_distance=2, prefix_length=7)
    benchmark(sym_spell.load_dictionary, dict_path, term_index=0, count_index=1, separator=" ")

def test_load_dict_load_dict_symspellcpppy(benchmark):
    symSpell = SymSpellCpp(max_dictionary_edit_distance=2, prefix_length=7)
    benchmark(symSpell.load_dictionary, dict_path, term_index=0, count_index=1, sep=" ")
