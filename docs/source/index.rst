SymSpellCppPy
=============

SymSpellCppPy corrects misspelled words, repairs misplaced spaces, and inserts missing word boundaries. It runs the Symmetric Delete algorithm in C++ and accepts Unicode text.

Install
-------

.. code-block:: console

   python -m pip install SymSpellCppPy

The project supports CPython 3.10–3.15. English word and word-pair frequency lists are included in the package.

Try a lookup
------------

.. doctest::

   >>> from SymSpellCppPy import SymSpell, Verbosity
   >>> spell = SymSpell(max_dictionary_edit_distance=1)
   >>> spell.create_dictionary_entry("damen", 42)
   True
   >>> hit, = spell.lookup("dämen", Verbosity.TOP)
   >>> hit.term, hit.distance, hit.count
   ('damen', 1, 42)

.. toctree::
   :maxdepth: 2
   :caption: Guides

   Examples
   dictionaries
   snapshots
   compatibility

.. toctree::
   :maxdepth: 2
   :caption: Reference

   SymSpellCppPy
   benchmarks
