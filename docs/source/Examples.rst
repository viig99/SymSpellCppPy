Correction and segmentation
===========================

Choose a method based on what you want to correct.

.. list-table::
   :header-rows: 1
   :widths: 34 33 33

   * - Method
     - Example input
     - Example result
   * - :meth:`~SymSpellCppPy.SymSpell.lookup`
     - ``recieve``
     - ``receive``
   * - :meth:`~SymSpellCppPy.SymSpell.lookup_compound`
     - ``the quick brown fox jumps overthe lazy dog``
     - ``the quick brown fox jumps over the lazy dog``
   * - :meth:`~SymSpellCppPy.SymSpell.word_segmentation`
     - ``Scientiﬁc-research``
     - ``Scientific research``

Run with the bundled English dictionaries
-----------------------------------------

The unigram list supplies spelling candidates and frequencies. The bigram list
helps compound lookup rank alternative splits.

.. literalinclude:: ../../examples/quickstart.py
   :language: python

Rank suggestions
----------------

``TOP`` returns at most one result. ``CLOSEST`` returns all suggestions at the
smallest distance found. ``ALL`` returns every suggestion within the distance
limit. Results are ranked by ascending edit distance, then descending frequency.

.. doctest::

   >>> from SymSpellCppPy import SymSpell, Verbosity
   >>> spell = SymSpell(max_dictionary_edit_distance=2)
   >>> for term, count in [("cat", 40), ("hat", 20), ("car", 30)]:
   ...     _ = spell.create_dictionary_entry(term, count)
   >>> [(item.term, item.distance) for item in spell.lookup("bat", Verbosity.ALL)]
   [('cat', 1), ('hat', 1), ('car', 2)]
   >>> spell.lookup("unlisted", Verbosity.TOP)
   []
   >>> spell.lookup("unlisted", Verbosity.TOP, include_unknown=True)[0].term
   'unlisted'
   >>> spell.lookup("Catt", Verbosity.TOP, transfer_casing=True)[0].term
   'Cat'

``max_edit_distance`` defaults to the index limit set at construction. A
smaller value can narrow individual lookups. With ``include_unknown=True``,
the fallback has count 0 and distance one above the lookup limit.

Segment text
------------

Pass ``max_edit_distance=0`` to insert word boundaries with exact dictionary
matches. The result contains the segmented input, the corrected output, and
the edit and probability scores.

.. doctest::

   >>> spell = SymSpell()
   >>> _ = spell.create_dictionary_entry("scientific", 10000)
   >>> _ = spell.create_dictionary_entry("research", 10000)
   >>> result = spell.word_segmentation("Scientiﬁc-research", max_edit_distance=0)
   >>> result.corrected_string
   'Scientific research'
