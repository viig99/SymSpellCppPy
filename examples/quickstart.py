"""Run with python examples/quickstart.py after installing SymSpellCppPy."""

from importlib.resources import as_file, files

from SymSpellCppPy import SymSpell, Verbosity

spell = SymSpell(max_dictionary_edit_distance=2)
data = files("SymSpellCppPy")

with as_file(data.joinpath("frequency_dictionary_en_82_765.txt")) as words:
    if not spell.load_dictionary(words, term_index=0, count_index=1):
        raise RuntimeError(f"Could not load {words}")

with as_file(data.joinpath("frequency_bigramdictionary_en_243_342.txt")) as pairs:
    if not spell.load_bigram_dictionary(pairs, term_index=0, count_index=2):
        raise RuntimeError(f"Could not load {pairs}")

suggestions = spell.lookup("recieve", Verbosity.TOP)
print(suggestions[0].term)  # receive

suggestions = spell.lookup_compound("the quick brown fox jumps overthe lazy dog")
print(suggestions[0].term)  # the quick brown fox jumps over the lazy dog

result = spell.word_segmentation("Scientiﬁc-research", max_edit_distance=0)
print(result.corrected_string)  # Scientific research
