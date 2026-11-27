Save and restore the index
==========================

A binary snapshot contains the searchable index, constructor settings, bigrams,
and counts that have yet to reach the indexing threshold. Load snapshots created
by your application or another trusted source.

.. doctest::

   >>> from SymSpellCppPy import SymSpell, Verbosity
   >>> original = SymSpell(max_dictionary_edit_distance=1, prefix_length=5)
   >>> original.create_dictionary_entry("damen", 42)
   True
   >>> restored = SymSpell()
   >>> restored.load_pickle_bytes(original.save_pickle_bytes())
   >>> restored.prefix_length()
   5
   >>> restored.lookup("dämen", Verbosity.TOP)[0].term
   'damen'

Use :meth:`~SymSpellCppPy.SymSpell.save_pickle` and
:meth:`~SymSpellCppPy.SymSpell.load_pickle` for files. Saving overwrites the
destination. The methods use the project's binary format.

Loading a snapshot from 0.0.18 or earlier rebuilds its Unicode index using the
receiver's constructor settings. Older snapshots contain the indexed words
but omit the original configuration, bigrams, and below-threshold counts.
After loading one, save it again to use the current format.
