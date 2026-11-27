# SymSpellCppPy

Fast Unicode spelling correction and word segmentation for Python. SymSpellCppPy uses the Symmetric Delete algorithm in C++20 and exposes it through nanobind.

[![PyPI](https://img.shields.io/pypi/v/SymSpellCppPy)](https://pypi.org/project/SymSpellCppPy/)
[![CI](https://github.com/viig99/SymSpellCppPy/actions/workflows/ci.yml/badge.svg)](https://github.com/viig99/SymSpellCppPy/actions/workflows/ci.yml)
[![Documentation (latest)](https://img.shields.io/readthedocs/symspellcpppy/latest)](https://symspellcpppy.readthedocs.io/en/latest/)

## Install

```sh
python -m pip install SymSpellCppPy
```

The project targets CPython 3.10–3.15. The release workflow builds wheels for Linux x86-64 and arm64, Windows x64, and macOS Intel and Apple Silicon. Source builds require CMake 3.26 or newer and a C++20 compiler.

## Correct a word

```python
from SymSpellCppPy import SymSpell, Verbosity

spell = SymSpell(max_dictionary_edit_distance=1)
spell.create_dictionary_entry("damen", 42)
spell.create_dictionary_entry("receive", 100)

suggestion, = spell.lookup("dämen", Verbosity.TOP)
print(suggestion.term, suggestion.distance)  # damen 1

suggestion, = spell.lookup("Recieve", Verbosity.TOP, transfer_casing=True)
print(suggestion.term)  # Receive
```

The package includes English unigram and bigram dictionaries. The [runnable quickstart](examples/quickstart.py) shows how to load them and use word lookup, compound correction, and segmentation. See the [usage guides and API reference](https://symspellcpppy.readthedocs.io/en/latest/) for custom dictionaries, Unicode, and binary snapshots.

## SymSpell compatibility

The [compatibility guide](docs/source/compatibility.rst) tracks the C# reference through SymSpell 6.7.3. It covers the 6.7 segmentation changes, the 6.7.1 dictionary update, and remaining differences with the reference and `symspellpy`.

## Development

```sh
python -m pip install -e '.[test,docs]'
python -m pytest
python -m sphinx -W -b html docs/source build/docs
python -m sphinx -W -b doctest docs/source build/doctest
```

Native tests use Catch2 and CTest.

```sh
cmake -S . -B build/native -DBUILD_FOR_TEST=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build/native --config Release
ctest --test-dir build/native -C Release --output-on-failure
```

Set the release version in `pyproject.toml`. The installed package exposes the same version as `SymSpellCppPy.__version__`.

1. Push a matching tag such as `v0.1.0` to build and test the wheels and source distribution. GitHub Actions stores them as artifacts for review.
2. Publish a GitHub release from that tag. The release run rebuilds and tests the distributions, then uploads them to PyPI.

## Benchmarks

See [benchmark results and reproduction commands](docs/source/benchmarks.rst). Run `python benchmark.py` to measure the installed extension. Install `.[benchmark]` and pass `--implementation python` to run the shared workloads against `symspellpy`.

## Acknowledgements

- [SymSpell by Wolf Garbe](https://github.com/wolfgarbe/SymSpell)
- [SymspellCPP by AtheS21](https://github.com/AtheS21/SymspellCPP)
- [symspellpy by mammothb](https://github.com/mammothb/symspellpy)
