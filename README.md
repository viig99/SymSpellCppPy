# SymSpellCppPy
A Fast SymSpell v6.5 port for python written in C++ using pybind11.

![UnitTests](https://img.shields.io/github/workflow/status/viig99/SymSpellCppPy/UnitTests?style=flat-square)
![Docs](https://img.shields.io/readthedocs/symspellcpppy?style=flat-square)
![Downloads](https://img.shields.io/pypi/dm/SymSpellCppPy?style=flat-square)
![License](https://img.shields.io/github/license/viig99/SymSpellCppPy?style=flat-square)

## Installation
```shell script
pip install --upgrade SymSpellCppPy
```

## Documentation
* Check [examples](https://symspellcpppy.readthedocs.io/en/latest/Examples.html) for provided usage.
* Check [api docs](https://symspellcpppy.readthedocs.io/en/latest/SymSpellCppPy.html#pybind11-binding-for-symspellpy) for detailed API documentation.
* Check `tests/SymSpellCppPyTest.py` for extended api usage.

## Benchmark Results
```shell script
pip install pytest pytest-benchmark symspellpy SymSpellCppPy
pytest benchmark.py --benchmark-compare
```
![Benchmark Results](https://github.com/viig99/SymSpellCppPy/blob/master/resources/benchmark.png?raw=true)

## Development
```shell script
git clone git@github.com:viig99/SymSpellCppPy.git
cd SymSpellCppPy
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Testing


### Python Bindings

### Build
```shell script
python3 setup.py build
```

### Test
#### Python tests
```shell script
python3 setup.py test
```

#### C++ tests
For detailed list of command line test flags please refer to [Catch 2 Docs](https://github.com/catchorg/Catch2/blob/master/docs/command-line.md#top)
```shell script
cd build
./Catch2Test -s
```

## Acknowledgements
* [SymSpell by @wolfgarbe](https://github.com/wolfgarbe/SymSpell)
* [SymspellCPP by @AtheS21](https://github.com/AtheS21/SymspellCPP)