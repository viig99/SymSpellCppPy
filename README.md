# SymSpellCppPy
A Fast SymSpell v6.5 port for python written in C++ using pybind11.

![UnitTests](https://github.com/viig99/SymSpellCppPy/workflows/UnitTests/badge.svg)

## Installation
```shell script
pip install SymSpellCppPy
```

## Usage
[Examples](https://symspellcpppy.readthedocs.io/en/latest/Examples.html)

## Development
```shell script
git clone git@github.com:viig99/SymSpellCppPy.git
cd SymSpellCppPy
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Testing
For detailed list of command line test flags please refer to [Catch 2 Docs](https://github.com/catchorg/Catch2/blob/master/docs/command-line.md#top)
```shell script
cd build
./Catch2Test -s
```

## Python Bindings

### Building
```shell script
python3 setup.py build
```

### Testing
```shell script
python3 setup.py test
```

## Acknowledgements
* [SymSpell by @wolfgarbe](https://github.com/wolfgarbe/SymSpell)
* [SymspellCPP by @AtheS21](https://github.com/AtheS21/SymspellCPP)