# SymSpellCppPy
A Fast SymSpell port for python written in C++ using pybind11.

## Installation
```shell script
git clone git@github.com:viig99/SymSpellCppPy.git
cd SymSpellCppPy
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Testing
For detailed list of command line test flags please refer to [Catch 2 Docs](https://github.com/catchorg/Catch2/blob/master/docs/command-line.md#top)
```shell script
cd build
./Catch2Test -s
```
