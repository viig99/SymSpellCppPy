Upstream compatibility
======================

SymSpellCppPy implements the main C# SymSpell algorithms and follows the
observable features in the `SymSpell 6.7.3 release
<https://github.com/wolfgarbe/SymSpell/releases/tag/v6.7.3>`_.

.. list-table::
   :header-rows: 1
   :widths: 15 43 42

   * - Upstream
     - Feature
     - SymSpellCppPy
   * - 6.7
     - Segmentation case, punctuation, ligatures, hyphenation, and US spellings
     - Initial capitalization is retained, punctuation and short apostrophe
       suffixes stay adjacent, and NFKC expands ligatures. ASCII hyphens are
       removed before segmentation. The English dictionary includes US variants.
   * - 6.7.1
     - More common contractions in the English dictionary
     - The bundled unigram dictionary is refreshed from the 6.7.3 tag.
   * - 6.7.2
     - Segmentation exception fix
     - Empty input and empty dictionaries are handled. Invalid distance and
       segmentation limits raise ``ValueError``.
   * - 6.7.3
     - Safe commit of an empty staging index
     - Empty commits do nothing.
   * - Compound lookup
     - Rank splits using bigrams and estimated frequencies
     - An observed bigram supplies its count. With no matching bigram, the
       product of unigram probabilities estimates its frequency.

The Python API includes corpus creation, word and bigram loading from files and
streams, dictionary updates, threshold purging, all three lookup modes, and
segmentation. Python also exposes casing transfer and binary snapshots.

The C# console application, .NET targets, and mutable public staging and bigram
collections have no Python equivalent. The reference exposes a mutable corpus
size for its frequency estimates. This port uses the default corpus size. The
Python loaders take one separator
character, with whitespace splitting by default. Unicode distance is measured
in code points. C# strings use UTF-16 code units. Ties and casing can differ
between ports.

The independent ``symspellpy`` port also offers regex token exclusions,
``ignore_non_words``, and selectable distance algorithms. SymSpellCppPy
currently uses Damerau optimal string alignment and provides its own snapshot
format.

Changes when upgrading from 0.0.18
----------------------------------

- CPython 3.10 is the minimum supported version.
- ``create_dictionary_entry`` returns true only when a term is newly added to
  the searchable index. Updating the count of an existing term returns false.
- ``save_pickle`` overwrites the destination. The reader accepts legacy archives.
