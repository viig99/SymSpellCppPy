Benchmarks
==========

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

Reproduce
---------

Run the benchmark from the repository checkout. It prints JSON with
the environment, median, and minimum for each workload.

.. code-block:: console

   python benchmark.py --rounds 7

To compare two installed versions, run once in each Python environment and
pass the same directory of frequency lists to ``--resources``. Pass
``--queries`` to use a separate query file. Each line's first whitespace-delimited
field is treated as a query.

The independent ``symspellpy`` package can run the shared workloads.

.. code-block:: console

   python -m pip install '.[benchmark]'
   python benchmark.py --implementation python --rounds 7
