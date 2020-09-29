"""
    To Check install
    pip install pytest pytest-benchmark symspellpy SymSpellCppPy
    pytest benchmark.py
"""

from symspellpy import SymSpell as SymSpellPy, Verbosity as VerbosityPy
from SymSpellCppPy import SymSpell as SymSpellCpp, Verbosity as VerbosityCpp
import pytest

dict_path = "resources/frequency_dictionary_en_82_765.txt"


@pytest.mark.benchmark(
    group="load_dict",
    min_rounds=5,
    disable_gc=True,
    warmup=False
)
def test_load_dict_symspellpy(benchmark):
    sym_spell = SymSpellPy(max_dictionary_edit_distance=2, prefix_length=7)
    benchmark(sym_spell.load_dictionary, dict_path, term_index=0, count_index=1, separator=" ")


@pytest.mark.benchmark(
    group="load_dict",
    min_rounds=5,
    disable_gc=True,
    warmup=False
)
def test_load_dict_symspellcpppy(benchmark):
    sym_spell = SymSpellCpp(max_dictionary_edit_distance=2, prefix_length=7)
    benchmark(sym_spell.load_dictionary, dict_path, term_index=0, count_index=1, separator=" ")


@pytest.mark.benchmark(
    group="lookup",
    min_rounds=5,
    disable_gc=True,
    warmup=False
)
def test_lookup_term_symspellpy(benchmark):
    sym_spell = SymSpellPy(max_dictionary_edit_distance=2, prefix_length=7)
    sym_spell.load_dictionary(dict_path, term_index=0, count_index=1, separator=" ")
    input_term = "mEmEbers"
    result = benchmark(sym_spell.lookup, input_term, VerbosityPy.CLOSEST, max_edit_distance=2, transfer_casing=True)
    assert (result[0].term.lower() == "members")


@pytest.mark.benchmark(
    group="lookup",
    min_rounds=5,
    disable_gc=True,
    warmup=False
)
def test_lookup_term_symspellcpppy(benchmark):
    sym_spell = SymSpellCpp(max_dictionary_edit_distance=2, prefix_length=7)
    sym_spell.load_dictionary(dict_path, term_index=0, count_index=1, separator=" ")
    input_term = "mEmEbers"
    result = benchmark(sym_spell.lookup, input_term, VerbosityCpp.CLOSEST, max_edit_distance=2)
    assert (result[0].term == "members")


@pytest.mark.benchmark(
    group="lookup_compound",
    min_rounds=5,
    disable_gc=True,
    warmup=False
)
def test_lookup_compound_term_symspellpy(benchmark):
    sym_spell = SymSpellPy(max_dictionary_edit_distance=2, prefix_length=7)
    sym_spell.load_dictionary(dict_path, term_index=0, count_index=1, separator=" ")
    input_term = "whereis th elove"
    result = benchmark(sym_spell.lookup_compound, input_term, max_edit_distance=2)
    assert (result[0].term == "whereas to love")


@pytest.mark.benchmark(
    group="lookup_compound",
    min_rounds=5,
    disable_gc=True,
    warmup=False
)
def test_lookup_compound_term_symspellcpppy(benchmark):
    sym_spell = SymSpellCpp(max_dictionary_edit_distance=2, prefix_length=7)
    sym_spell.load_dictionary(dict_path, term_index=0, count_index=1, separator=" ")
    input_term = "whereis th elove"
    result = benchmark(sym_spell.lookup_compound, input_term, max_edit_distance=2)
    assert (result[0].term == "whereas to love")


@pytest.mark.benchmark(
    group="word_segmentation",
    min_rounds=5,
    disable_gc=True,
    warmup=False
)
def test_word_segmentation_symspellpy(benchmark):
    sym_spell = SymSpellPy(max_dictionary_edit_distance=2, prefix_length=7)
    sym_spell.load_dictionary(dict_path, term_index=0, count_index=1, separator=" ")
    input_term = "thequickbrownfoxjumpsoverthelazydog"
    result = benchmark(sym_spell.word_segmentation, input_term, max_edit_distance=0, max_segmentation_word_length=5)
    assert (result.segmented_string == "t he quick brown fox jumps overt he lazy dog")


@pytest.mark.benchmark(
    group="word_segmentation",
    min_rounds=5,
    disable_gc=True,
    warmup=False
)
def test_word_segmentation_symspellcpppy(benchmark):
    sym_spell = SymSpellCpp(max_dictionary_edit_distance=2, prefix_length=7)
    sym_spell.load_dictionary(dict_path, term_index=0, count_index=1, separator=" ")
    input_term = "thequickbrownfoxjumpsoverthelazydog"
    result = benchmark(sym_spell.word_segmentation, input_term, max_edit_distance=0, max_segmentation_word_length=5)
    assert (result.segmented_string == "t he quick brown fox jumps overt he lazy dog")
