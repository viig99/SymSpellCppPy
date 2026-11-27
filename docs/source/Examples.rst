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

.. doctest:: ranking

   >>> from SymSpellCppPy import SymSpell, Verbosity
   >>> spell = SymSpell(max_dictionary_edit_distance=2)
   >>> for term, count in [("cat", 40), ("hat", 20), ("car", 30)]:
   ...     _ = spell.create_dictionary_entry(term, count)
   >>> [item.term for item in spell.lookup("bat", Verbosity.TOP)]
   ['cat']
   >>> [item.term for item in spell.lookup("bat", Verbosity.CLOSEST)]
   ['cat', 'hat']
   >>> [(item.term, item.distance) for item in spell.lookup("bat", Verbosity.ALL)]
   [('cat', 1), ('hat', 1), ('car', 2)]
   >>> [item.term for item in spell.lookup("bat", Verbosity.ALL, max_edit_distance=1)]
   ['cat', 'hat']
   >>> spell.lookup("unlisted", Verbosity.TOP)
   []
   >>> fallback, = spell.lookup("unlisted", Verbosity.TOP, include_unknown=True)
   >>> fallback.term, fallback.distance, fallback.count
   ('unlisted', 3, 0)
   >>> spell.lookup("Catt", Verbosity.TOP, transfer_casing=True)[0].term
   'Cat'

``max_edit_distance`` defaults to the index limit set at construction. A
smaller value can narrow individual lookups. With ``include_unknown=True``,
the fallback has count 0 and distance one above the lookup limit.

Repair misplaced spaces
-----------------------

Compound correction can join separated parts of a known word and split a
joined phrase. When two splits have the same edit distance, a bigram can
favor the phrase that appears in your corpus. Here, the unigram counts
initially favor ``no where``. The word-pair dictionary favors ``now here``.

.. doctest:: compound

   >>> from io import StringIO
   >>> from SymSpellCppPy import SymSpell
   >>> spell = SymSpell(max_dictionary_edit_distance=1)
   >>> for word in ("no", "where", "notebook"):
   ...     _ = spell.create_dictionary_entry(word, 100_000_000)
   >>> for word in ("now", "here"):
   ...     _ = spell.create_dictionary_entry(word, 10_000_000)
   >>> spell.lookup_compound("note book")[0].term
   'notebook'
   >>> spell.lookup_compound("nowhere")[0].term
   'no where'
   >>> pairs = StringIO("now here\t200000\n")
   >>> spell.load_bigram_dictionary_stream(pairs, 0, 1, separator="\t")
   True
   >>> spell.lookup_compound("Nowhere", transfer_casing=True)[0].term
   'Now here'

Segment text
------------

Pass ``max_edit_distance=0`` to insert word boundaries with exact dictionary
matches. The result contains the segmented input, the corrected output, and
the edit and probability scores.

.. doctest:: segmentation

   >>> from SymSpellCppPy import SymSpell
   >>> spell = SymSpell(max_dictionary_edit_distance=1)
   >>> _ = spell.create_dictionary_entry("scientific", 10000)
   >>> _ = spell.create_dictionary_entry("research", 10000)
   >>> result = spell.word_segmentation("Scientiﬁc-research", max_edit_distance=0)
   >>> result.corrected_string
   'Scientific research'

Use a positive edit distance to correct misspelled words during segmentation.
Set ``max_segmentation_word_length`` to limit the length of each segment the
algorithm considers. Compare both strings to see which words it corrected.

.. doctest:: segmentation

   >>> spell = SymSpell(max_dictionary_edit_distance=1)
   >>> _ = spell.create_dictionary_entry("hello", 10000)
   >>> _ = spell.create_dictionary_entry("world", 10000)
   >>> result = spell.word_segmentation(
   ...     "Hellowrld!", max_edit_distance=1, max_segmentation_word_length=5
   ... )
   >>> result.segmented_string, result.corrected_string, result.distance_sum
   ('Hello wrld!', 'Hello world!', 2)
