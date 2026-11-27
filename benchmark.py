"""Time a fixed set of SymSpell operations and emit JSON for comparison."""

import argparse
import json
import platform
import sys
from importlib.metadata import PackageNotFoundError, version
from pathlib import Path
from statistics import median
from time import perf_counter_ns

ROOT = Path(__file__).resolve().parent
CLEAN = "the quick brown fox jumps over the lazy dog"
SEGMENTATION = "thequickbrownfoxjumpsoverthelazydog"
COMPOUND = "the quick brown fox jumps overthe lazy dog"


def measure(operation, repeat, iterations):
    operation()
    samples = []
    for _ in range(repeat):
        start = perf_counter_ns()
        for _ in range(iterations):
            operation()
        samples.append((perf_counter_ns() - start) / iterations / 1_000)
    return {"median_us": round(median(samples), 3), "min_us": round(min(samples), 3)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--implementation", choices=["cpp", "python", "python-fast"], default="cpp",
        help="use SymSpellCppPy, symspellpy, or symspellpy with editdistpy",
    )
    parser.add_argument("--rounds", type=int, default=7)
    parser.add_argument("--resources", type=Path, default=ROOT / "resources")
    parser.add_argument("--queries", type=Path, default=ROOT / "tests/fortests/noisy_query_en_1000.txt")
    args = parser.parse_args()
    if args.rounds < 1:
        parser.error("--rounds must be positive")

    if args.implementation == "cpp":
        from SymSpellCppPy import SymSpell, Verbosity

        distribution = "SymSpellCppPy"
        new_index = SymSpell
        backend = "C++ Damerau OSA"
    else:
        from symspellpy import SymSpell, Verbosity
        from symspellpy.editdistance import DistanceAlgorithm, EditDistance

        distribution = "symspellpy"
        algorithm = DistanceAlgorithm.DAMERAU_OSA
        backend = "Python Damerau OSA"
        if args.implementation == "python-fast":
            try:
                accelerator_version = version("editdistpy")
            except PackageNotFoundError:
                parser.error("python-fast requires symspellpy[editdistpy]")
            algorithm = DistanceAlgorithm.DAMERAU_OSA_FAST
            backend = f"editdistpy {accelerator_version}"

        def new_index():
            return SymSpell(distance_comparer=EditDistance(algorithm))

    dictionary = str(args.resources / "frequency_dictionary_en_82_765.txt")
    bigrams = str(args.resources / "frequency_bigramdictionary_en_243_342.txt")
    queries = [line.split()[0] for line in args.queries.read_text(encoding="utf-8").splitlines() if line.strip()]

    def load_words():
        spell = new_index()
        assert spell.load_dictionary(dictionary, 0, 1)
        return spell

    def load_bigrams():
        assert new_index().load_bigram_dictionary(bigrams, 0, 2)

    spell = load_words()
    assert spell.load_bigram_dictionary(bigrams, 0, 2)
    assert spell.lookup("recieve", Verbosity.TOP)[0].term == "receive"
    assert spell.lookup_compound(COMPOUND, 2)[0].term == CLEAN
    assert spell.word_segmentation(SEGMENTATION).corrected_string == CLEAN
    assert spell.lookup("StreaM", Verbosity.TOP, 1, transfer_casing=True)[0].term == "StreaM"

    def lookup_batch():
        for query in queries:
            spell.lookup(query, Verbosity.CLOSEST, 2)

    operations = {
        "load_unigrams": (load_words, 1),
        "load_bigrams": (load_bigrams, 1),
        "lookup_exact": (lambda: spell.lookup("receive", Verbosity.TOP), 10_000),
        "lookup_typo": (lambda: spell.lookup("recieve", Verbosity.TOP), 2_000),
        "lookup_casing": (lambda: spell.lookup("RecIEve", Verbosity.TOP, 2, transfer_casing=True), 1_000),
        "lookup_noisy_queries": (lookup_batch, 3),
        "lookup_compound": (lambda: spell.lookup_compound(COMPOUND, 2), 100),
        "segment": (lambda: spell.word_segmentation(SEGMENTATION), 100),
    }
    if args.implementation == "cpp":
        payload = spell.save_pickle_bytes()
        operations["serialize"] = (spell.save_pickle_bytes, 1)
        operations["deserialize"] = (lambda: SymSpell().load_pickle_bytes(payload), 1)

    try:
        package_version = version(distribution)
    except PackageNotFoundError:
        package_version = "uninstalled extension"

    print(json.dumps({
        "implementation": distribution,
        "version": package_version,
        "distance_backend": backend,
        "python": sys.version.split()[0],
        "platform": platform.platform(),
        "rounds": args.rounds,
        "noisy_queries": len(queries),
        "archive_bytes": len(payload) if args.implementation == "cpp" else None,
        "results": {
            name: measure(operation, args.rounds, iterations)
            for name, (operation, iterations) in operations.items()
        },
    }, indent=2))


if __name__ == "__main__":
    main()
