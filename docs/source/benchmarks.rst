Benchmarks
==========

Comparison with other ports
---------------------------

These measurements compare the published SymSpellCppPy 0.1.0 wheel with
`symspellpy 6.10.2 <https://pypi.org/project/symspellpy/6.10.2/>`_ and
`symspell_rs 7.0.1 <https://github.com/wolfgarbe/symspell_rs/releases/tag/v7.0.1>`_.
All runs used the same English unigram and bigram files in this repository,
the same 1,000 noisy queries, a dictionary edit distance of 2, and a prefix
length of 7. Values are medians of seven rounds on one pinned AMD Ryzen 9 9900X
core on Linux x86-64. The Python runs used CPython 3.13.15. Rust used rustc 1.100.0
in release mode with link-time optimization.

Python API comparison
~~~~~~~~~~~~~~~~~~~~~

The accelerator column enables the optional Damerau OSA backend from
`editdistpy 0.5.0 <https://pypi.org/project/editdistpy/0.5.0/>`_. Dictionary
parsing, candidate generation, and ranking still run in Python in that mode.

.. list-table::
   :header-rows: 1
   :widths: 43 19 18 20

   * - Workload
     - SymSpellCppPy
     - symspellpy
     - symspellpy + editdistpy
   * - Load unigrams
     - 757 ms
     - 921 ms
     - 943 ms
   * - Load bigrams
     - 71.7 ms
     - 75.1 ms
     - 77.6 ms
   * - Exact lookup
     - 0.126 µs
     - 0.246 µs
     - 0.244 µs
   * - Spelling correction
     - 1.93 µs
     - 12.35 µs
     - 8.01 µs
   * - Correction with casing transfer
     - 3.03 µs
     - 18.34 µs
     - 13.62 µs
   * - 1,000 noisy queries
     - 5.27 ms
     - 39.30 ms
     - 19.28 ms
   * - Compound correction
     - 116 µs
     - 1,430 µs
     - 644 µs
   * - Word segmentation
     - 2.81 ms
     - 15.88 ms
     - 9.79 ms

The 1,000-query batch took 7.5 times as long with default symspellpy and 3.7
times as long with editdistpy as it did with SymSpellCppPy.

Native Rust reference
~~~~~~~~~~~~~~~~~~~~~

The Rust harness calls the crate directly, so its timings exclude Python
binding overhead. The gxhash column enables the upstream hash feature and
compiles for the host CPU to enable the required target features.

.. list-table::
   :header-rows: 1
   :widths: 43 19 18 20

   * - Workload
     - SymSpellCppPy via Python
     - symspell_rs default
     - symspell_rs with gxhash
   * - Load unigrams
     - 757 ms
     - 355 ms
     - 329 ms
   * - Load bigrams
     - 71.7 ms
     - 27.9 ms
     - 27.3 ms
   * - Exact lookup
     - 0.126 µs
     - 0.026 µs
     - 0.026 µs
   * - Spelling correction
     - 1.93 µs
     - 0.502 µs
     - 0.496 µs
   * - Correction with casing transfer
     - 3.03 µs
     - 0.559 µs
     - 0.547 µs
   * - 1,000 noisy queries
     - 5.27 ms
     - 2.07 ms
     - 2.01 ms
   * - Compound correction
     - 116 µs
     - 37.0 µs
     - 35.1 µs
   * - Word segmentation
     - 2.81 ms
     - 0.876 ms
     - 0.836 ms

Both harnesses check a spelling correction, compound correction, segmentation,
and casing before timing. These times measure representative operations.
Candidate lists and rankings can differ between ports.

Other upstream-listed Python ports have different support baselines. The
`zoho-labs bindings <https://pypi.org/project/symspell-rust/0.3.0/>`_ publish
a CPython 3.9 Windows wheel. The
`PySymSpell source <https://github.com/ne3x7/pysymspell/blob/021c81f30dcd1f3b092707ed20ff2894995ddf03/symspell/symspell.py>`_
raises a syntax error under the tested interpreter.

Changes from 0.0.18 to 0.1.0
----------------------------

SymSpellCppPy 0.0.18 and 0.1.0 were measured on the same AMD Ryzen 9 9900X,
using GCC 16.2 and CPython 3.13.15 on Linux x86-64. Both runs used the same
0.0.18 dictionaries and 1,000-query fixture. The baseline was built from the
0.0.18 source with its native CPU flags. The 0.1.0 wheel used its portable
release flags. Values are medians of seven rounds.

.. list-table::
   :header-rows: 1
   :widths: 52 24 24

   * - Workload
     - 0.0.18
     - 0.1.0
   * - Load the unigram dictionary
     - 759 ms
     - 777 ms
   * - Load the bigram dictionary
     - 57.5 ms
     - 77.0 ms
   * - Exact lookup
     - 0.222 µs
     - 0.116 µs
   * - Spelling correction
     - 1.74 µs
     - 1.88 µs
   * - Spelling correction with casing transfer
     - 3.56 µs
     - 2.95 µs
   * - 1,000 noisy queries
     - 5.38 ms
     - 5.23 ms
   * - Compound correction
     - 143 µs
     - 112 µs
   * - Word segmentation
     - 2.91 ms
     - 2.70 ms
   * - Save a snapshot to bytes
     - 118 ms
     - 128 ms
   * - Load a snapshot from bytes
     - 202 ms
     - 101 ms
   * - Snapshot size
     - 41.5 MiB
     - 33.4 MiB

The new snapshot also includes bigrams and the index configuration.
Each version loaded its own snapshot during the benchmark.

Reproduce the comparison
------------------------

Run the benchmark from the repository checkout. It prints JSON with
the environment, median, and minimum for each workload.

.. code-block:: console

   python -m pip install 'SymSpellCppPy==0.1.0' 'symspellpy[editdistpy]==6.10.2' 'editdistpy==0.5.0'
   python benchmark.py --implementation cpp --rounds 7
   python benchmark.py --implementation python --rounds 7
   python benchmark.py --implementation python-fast --rounds 7

To compare two installed versions, run once in each Python environment and
pass the same directory of frequency lists to ``--resources``. Pass
``--queries`` to use a separate query file. Each line's first whitespace-delimited
field is treated as a query.

The native harness prints tab-separated timings. It requires Cargo and Rust
1.91 or newer. Run the gxhash variant on hardware with the target features
required by that crate.

.. code-block:: console

   cargo run --locked --release --manifest-path benchmarks/rust/Cargo.toml -- \
       resources tests/fortests/noisy_query_en_1000.txt 7
   RUSTFLAGS='-C target-cpu=native' cargo run --locked --release --features gxhash \
       --manifest-path benchmarks/rust/Cargo.toml -- resources tests/fortests/noisy_query_en_1000.txt 7
