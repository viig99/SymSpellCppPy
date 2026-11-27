Dictionaries
============

Build a project vocabulary
--------------------------

Call :meth:`~SymSpellCppPy.SymSpell.create_dictionary_entry` to add a count to
a term. It returns true when the term crosses the indexing threshold for the
first time. Counts below the threshold accumulate until they reach it.

.. doctest::

   >>> from SymSpellCppPy import SymSpell, Verbosity
   >>> spell = SymSpell(count_threshold=3)
   >>> spell.create_dictionary_entry("nanobind", 2)
   False
   >>> spell.create_dictionary_entry("nanobind", 1)
   True
   >>> spell.lookup("nanobnd", Verbosity.TOP)[0].term
   'nanobind'
   >>> spell.delete_dictionary_entry("nanobind")
   True
   >>> spell.word_count()
   0

Load a UTF-8 word list
---------------------

Pass a string path or a path-like object. Column indices start at zero.
With the default separator, runs of whitespace separate columns. To load a
tab-separated pair and count, pass ``separator="\t"`` and use columns 0 and 1.

.. doctest::

   >>> from io import StringIO
   >>> spell = SymSpell(max_dictionary_edit_distance=1)
   >>> words = StringIO("damen\t42\nélan\t7\n")
   >>> spell.load_dictionary_stream(words, 0, 1, separator="\t")
   True
   >>> spell.lookup("élan", Verbosity.TOP)[0].count
   7

In a space-separated bigram file, the default format is ``word1 word2 count``.
Use ``term_index=0, count_index=2``. With an explicit separator, put the whole
word pair in one column. Unigram rows with a word and a missing or invalid
count default to a count of 1. Bigram rows with invalid counts and rows
with malformed UTF-8 are skipped.

Use :meth:`~SymSpellCppPy.SymSpell.create_dictionary` instead to build counts
from plain text. The ``*_stream`` methods accept objects with a ``read()``
method returning text or UTF-8 bytes. Use the path methods for large files to
read them one line at a time.

Use Unicode text
----------------

Edit distance counts Unicode code points. An umlaut or an emoji takes one
position, including on Windows. Word segmentation applies Unicode NFKC
normalization and removes ASCII hyphens before scoring. Direct word lookup
uses the supplied code points, so normalize both the dictionary and queries
before lookup if your data mixes composed and decomposed spellings.
