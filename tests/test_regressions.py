import io
from importlib.resources import as_file, files
from pathlib import Path

import pytest

from SymSpellCppPy import ALL, CLOSEST, TOP, Info, SuggestItem, SymSpell, Verbosity, __version__


@pytest.fixture(scope="module")
def english():
    spell = SymSpell()
    with as_file(files("SymSpellCppPy").joinpath("frequency_dictionary_en_82_765.txt")) as path:
        assert spell.load_dictionary(path, 0, 1)
    return spell


def test_public_binding_exports():
    assert (TOP, CLOSEST, ALL) == (Verbosity.TOP, Verbosity.CLOSEST, Verbosity.ALL)
    assert int(Verbosity.CLOSEST) == 1
    assert __version__
    item = SuggestItem("damen", 1, 42)
    assert (item == "damen") is False
    assert item < SuggestItem("dramen", 2, 50)
    result = Info()
    assert (result.segmented_string, result.corrected_string, result.distance_sum, result.log_prob_sum) == (
        "", "", 0, 0.0
    )
    result.set("ä", "Ä", 1, -2.0)
    assert (result.get_segmented(), result.get_corrected(), result.get_distance(), result.get_probability()) == (
        "ä", "Ä", 1, -2.0
    )


@pytest.mark.parametrize("word, query", [("damen", "dämen"), ("A𐐀", "A𐐁"), ("tea🍵", "tea🫖")])
def test_edit_distance_counts_code_points(word, query):
    spell = SymSpell(max_dictionary_edit_distance=1, prefix_length=5)
    spell.create_dictionary_entry(word, 42)
    hit, = spell.lookup(query, TOP, transfer_casing=True)
    assert (hit.term, hit.distance, hit.count) == (word, 1, 42)
    assert spell.max_length() == len(word)


def test_utf8_paths_and_dictionary_separators(tmp_path):
    path = tmp_path / "wörter🫖.txt"
    path.write_bytes("\ufeffdamen«42\r\nélite«3\r\n".encode())
    spell = SymSpell(1, 5)
    assert spell.load_dictionary(path, 0, 1, separator="«")
    assert spell.lookup("dämen", TOP)[0].count == 42
    assert spell.lookup("élit", TOP)[0].term == "élite"


def test_dictionary_row_validation():
    spell = SymSpell()
    with pytest.raises(ValueError, match="column"):
        spell.load_dictionary_stream(io.StringIO("tea 10"), -1, 1)
    with pytest.raises(ValueError, match="separator"):
        spell.load_dictionary_stream(io.StringIO("tea 10"), 0, 1, separator="  ")
    assert spell.load_dictionary_stream(io.BytesIO(
        b"\xef\xbb\xbftea\t 10\r\n\n\xffinvalid\ncoffee 999999999999999999999999\n"
    ), 0, 1)
    assert spell.lookup("tea", TOP)[0].count == 10
    assert spell.lookup("coffee", TOP)[0].count == 1
    assert spell.word_count() == 2
    assert not spell.load_bigram_dictionary_stream(io.StringIO("missing count\ncoffee tea invalid\n"), 0, 2)
    assert spell.create_dictionary_stream(io.StringIO("Élan ÉLAN"))
    assert spell.lookup("élan", TOP)[0].count == 2
    assert spell.delete_dictionary_entry("ÉLAN")


def test_reported_compound_casing_corruption(english):
    for query in ("utuxgent", "spswattle", "forgertrsts", "rollelicy", "cicuris"):
        original, = english.lookup_compound(query, max_edit_distance=1)
        corrected, = english.lookup_compound(query, max_edit_distance=1, transfer_casing=True)
        assert corrected == original

    spell = SymSpell()
    spell.create_dictionary_entry("a", 1_000_000)
    assert spell.lookup_compound(">^v<G", 1, True)[0].term == "a a"
    assert spell.lookup_compound("8Qp>=U", 0, True)[0].term == "8Qp U"


def test_symspell_67_segmentation(english):
    assert english.word_segmentation("Scien-tiﬁc,research", 0).corrected_string == "Scientific, research"
    assert english.word_segmentation("Hello,world!").corrected_string == "Hello, world!"
    assert english.word_segmentation("Hello,world!").segmented_string == "Hello, world!"
    assert english.word_segmentation("Theuser’sresearch", 0).corrected_string == "The user’s research"
    for word in ("favourable", "favorable", "doesn't"):
        assert english.lookup(word, TOP, 0)[0].term == word


def test_distance_defaults_and_empty_index():
    spell = SymSpell(max_dictionary_edit_distance=0)
    assert spell.lookup("Unknown", TOP, include_unknown=True, transfer_casing=True) == [
        SuggestItem("Unknown", 1, 0)
    ]
    for lookup in (
        lambda distance: spell.lookup("", TOP, distance),
        lambda distance: spell.lookup_compound("", distance),
        lambda distance: spell.word_segmentation("", distance),
    ):
        with pytest.raises(ValueError):
            lookup(-1)
        with pytest.raises(ValueError):
            lookup(1)
    assert spell.word_segmentation("").distance_sum == 0
    assert spell.word_segmentation(" \t ").corrected_string == ""
    assert spell.word_segmentation("unknown").corrected_string == "unknown"
    with pytest.raises(ValueError):
        spell.word_segmentation("unknown", max_segmentation_word_length=0)


def test_removing_words_cleans_all_delete_buckets():
    spell = SymSpell(max_dictionary_edit_distance=2, compact_level=16)
    spell.create_dictionary_entry("staging", 100)
    assert spell.entry_count() > 0
    assert spell.delete_dictionary_entry("staging")
    assert spell.entry_count() == 0
    assert spell.max_length() == 0
    assert spell.lookup("stagng", ALL) == []
    assert spell.word_count() == 0
    assert spell.create_dictionary_entry("staging", 4)
    assert spell.lookup("stagng", ALL) == [SuggestItem("staging", 1, 4)]


def test_rank_uses_distance_before_frequency():
    spell = SymSpell()
    spell.create_dictionary_entry("cot", 1000)
    spell.create_dictionary_entry("cat", 1)
    spell.create_dictionary_entry("bat", 10)
    assert [hit.term for hit in spell.lookup("cat", ALL)] == ["cat", "cot", "bat"]
    assert [hit.term for hit in spell.lookup("hat", CLOSEST)] == ["bat", "cat"]


def test_compound_frequency_uses_64_bits():
    limit = (1 << 63) - 1
    spell = SymSpell()
    spell.create_dictionary_entry("hello", limit)
    spell.create_dictionary_entry("world", limit)
    assert spell.lookup_compound("hello world")[0].count == limit


def test_snapshot_overwrite_preserves_settings_and_counts(tmp_path):
    original = SymSpell(1, 5, 3, initial_capacity=5, compact_level=16)
    original.create_dictionary_entry("damen", 42)
    original.create_dictionary_entry("élan", 2)
    for word in ("foo", "bar"):
        original.create_dictionary_entry(word, 300_000)
    assert original.load_bigram_dictionary_stream(io.StringIO("foo bar 600000\n"), 0, 2)

    path = tmp_path / "modèle🫖.bin"
    original.save_pickle(path)
    assert original.delete_dictionary_entry("damen")
    original.save_pickle(path)
    restored = SymSpell()
    restored.load_pickle(path)

    assert (restored.max_dictionary_edit_distance(), restored.prefix_length(), restored.count_threshold()) == (1, 5, 3)
    assert restored.lookup("dämen", TOP) == []
    assert restored.lookup_compound("foobar")[0].term == "foo bar"
    assert restored.lookup_compound("foobar")[0].count == 600_000
    assert restored.create_dictionary_entry("élan", 1)
    assert restored.lookup("élan", TOP)[0].count == 3


def test_legacy_snapshot_rebuilds_unicode_index():
    # Produced by 0.0.18's save_pickle_bytes() using "damen" (42) and "élan" (7).
    fixture = Path(__file__).with_name("fortests") / "legacy_0_0_18.bin"
    spell = SymSpell(1, 5, count_threshold=100)
    spell.load_pickle_bytes(memoryview(fixture.read_bytes()))
    assert spell.count_threshold() == 100
    assert spell.lookup("dämen", TOP)[0] == SuggestItem("damen", 1, 42)
    assert spell.lookup("élan", TOP)[0] == SuggestItem("élan", 0, 7)


def test_bad_snapshot_does_not_replace_live_index():
    spell = SymSpell()
    spell.create_dictionary_entry("present", 20)
    payload = spell.save_pickle_bytes()
    for invalid in (b"", payload[:-1]):
        with pytest.raises(RuntimeError):
            spell.load_pickle_bytes(invalid)
        assert spell.lookup("present", TOP)[0].count == 20
    with pytest.raises(ValueError, match="contiguous"):
        spell.load_pickle_bytes(memoryview(payload)[::2])
