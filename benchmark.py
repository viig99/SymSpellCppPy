'''
    To Check install
    pip install pytest pytest-benchmark symspellpy SymSpellCppPy
    pytest benchmark.py
'''

from symspellpy import SymSpell, Verbosity
from SymSpellCppPy import SymSpell as SymSpellCpp, Verbosity as VerbosityCpp
import time
import pytest

dict_path = "resources/frequency_dictionary_en_82_765.txt"

@pytest.mark.benchmark(
    group="load_dict",
    min_rounds=5,
    disable_gc=True,
    warmup=False
)
def test_load_dict_symspellpy(benchmark):
    sym_spell = SymSpell(max_dictionary_edit_distance=2, prefix_length=7)
    benchmark(sym_spell.load_dictionary, dict_path, term_index=0, count_index=1, separator=" ")

@pytest.mark.benchmark(
    group="load_dict",
    min_rounds=5,
    disable_gc=True,
    warmup=False
)
def test_load_dict_symspellcpppy(benchmark):
    symSpell = SymSpellCpp(max_dictionary_edit_distance=2, prefix_length=7)
    benchmark(symSpell.load_dictionary, dict_path, term_index=0, count_index=1, sep=" ")

@pytest.mark.benchmark(
    group="lookup",
    min_rounds=5,
    disable_gc=True,
    warmup=False
)
def test_lookup_term_symspellpy(benchmark):
    sym_spell = SymSpell(max_dictionary_edit_distance=2, prefix_length=7)
    sym_spell.load_dictionary(dict_path, term_index=0, count_index=1, separator=" ")
    input_term = "mEmEbers"
    benchmark(sym_spell.lookup, input_term, Verbosity.CLOSEST, max_edit_distance=2, transfer_casing=True)

@pytest.mark.benchmark(
    group="lookup",
    min_rounds=5,
    disable_gc=True,
    warmup=False
)
def test_lookup_term_symspellcpppy(benchmark):
    sym_spell = SymSpellCpp(max_dictionary_edit_distance=2, prefix_length=7)
    sym_spell.load_dictionary(dict_path, term_index=0, count_index=1, sep=" ")
    input_term = "mEmEbers"
    benchmark(sym_spell.lookup_term, input_term.lower(), VerbosityCpp.Closest, max_edit_distance=2)
