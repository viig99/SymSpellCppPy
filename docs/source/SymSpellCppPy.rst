Python API reference
====================

.. py:module:: SymSpellCppPy

``SymSpell``
------------

.. py:class:: SymSpell(max_dictionary_edit_distance=2, prefix_length=7, count_threshold=1, initial_capacity=82765, compact_level=5)

   A mutable dictionary and its delete index.

   :param max_dictionary_edit_distance: Maximum edit distance supported by the index.
   :param prefix_length: Number of leading code points used to generate deletes. It must exceed the distance limit.
   :param count_threshold: Minimum frequency required for a word to appear in suggestions.
   :param initial_capacity: Expected number of words. This is an allocation hint.
   :param compact_level: Hash compaction from 0 to 16. Larger values favor lower memory use over speed.

Correction
~~~~~~~~~~

.. py:method:: SymSpell.lookup(input, verbosity, max_edit_distance=None, include_unknown=False, transfer_casing=False)

   Return a list of :class:`SuggestItem` objects. Choose a :class:`Verbosity`
   level to control the number of results. When ``include_unknown`` is true,
   a query with no match returns a fallback with count 0 and distance one
   above the requested limit. Enable ``transfer_casing`` to carry casing from
   the input to the suggested term.

.. py:method:: SymSpell.lookup_compound(input, max_edit_distance=None, transfer_casing=False)

   Return one :class:`SuggestItem` for the phrase. This method can split
   joined words and merge words with an extra space. Its distance is measured
   across the full input and can exceed the per-word lookup limit.

.. py:method:: SymSpell.word_segmentation(input, max_edit_distance=None, max_segmentation_word_length=None)

   Return an :class:`Info` result with inserted word boundaries and optional
   spelling corrections. When omitted, the word length limit comes from the
   longest indexed term.

Dictionary management
~~~~~~~~~~~~~~~~~~~~~

.. py:method:: SymSpell.load_dictionary(corpus, term_index, count_index, separator=" ")

   Merge terms and frequencies from a UTF-8 path. Column indices are zero-based.
   Return true when the dictionary contains indexed words after loading.

.. py:method:: SymSpell.load_bigram_dictionary(corpus, term_index, count_index, separator=" ")

   Merge word-pair frequencies from a UTF-8 path. With the default separator,
   two consecutive columns form the pair. With a custom separator the pair
   occupies one column. Return true when at least one row was loaded.

.. py:method:: SymSpell.create_dictionary(corpus)

   Count tokens in a plain UTF-8 text file and merge them into the dictionary.

.. py:method:: SymSpell.load_dictionary_stream(corpus, term_index, count_index, separator=" ")
               SymSpell.load_bigram_dictionary_stream(corpus, term_index, count_index, separator=" ")
               SymSpell.create_dictionary_stream(corpus)

   Stream variants of the three path methods. A stream's ``read()`` method
   must return text or UTF-8 bytes.

.. py:method:: SymSpell.create_dictionary_entry(key, count)

   Lowercase the key and add the supplied count. Return true if the word was
   newly indexed. An update to an existing word can change its frequency and
   still return false.

.. py:method:: SymSpell.delete_dictionary_entry(key)

   Remove an indexed term and its delete entries. Return true if the key existed.

.. py:method:: SymSpell.purge_below_threshold_words()

   Discard accumulated counts for terms that have yet to reach the threshold.

Inspect the index
~~~~~~~~~~~~~~~~~

.. py:method:: SymSpell.word_count()

   Return the number of indexed terms.

.. py:method:: SymSpell.entry_count()

   Return the number of delete hash buckets.

.. py:method:: SymSpell.max_length()

   Return the longest indexed term's length in code points.

.. py:method:: SymSpell.max_dictionary_edit_distance()
               SymSpell.prefix_length()
               SymSpell.count_threshold()

   Return the corresponding index setting.

Snapshots
~~~~~~~~~

.. py:method:: SymSpell.save_pickle(filepath)
               SymSpell.load_pickle(filepath)

   Write or restore a dictionary snapshot at a path. Saving overwrites the
   destination. See :doc:`snapshots` for version compatibility.

.. py:method:: SymSpell.save_pickle_bytes()
               SymSpell.load_pickle_bytes(bytes)

   Write a snapshot to bytes or restore one from a one-dimensional,
   contiguous buffer.

Result types
------------

.. py:class:: Verbosity

   Controls filtering in :meth:`SymSpell.lookup`.

   .. py:attribute:: TOP

      The highest-frequency candidate at the smallest edit distance.

   .. py:attribute:: CLOSEST

      All candidates at the smallest edit distance.

   .. py:attribute:: ALL

      All candidates within the requested distance.

.. py:class:: SuggestItem(term, distance, count)

   A suggestion returned by a lookup.

   .. py:attribute:: term

      The suggested spelling, as a string.

   .. py:attribute:: distance

      The edit distance to the query.

   .. py:attribute:: count

      The dictionary frequency, or an estimated count for compound correction.

.. py:class:: Info()

   A result from :meth:`SymSpell.word_segmentation`.

   .. py:attribute:: segmented_string

      The input after normalization and insertion of word boundaries.

   .. py:attribute:: corrected_string

      The normalized, segmented, and corrected text.

   .. py:attribute:: distance_sum

      Total edit cost of the chosen segmentation.

   .. py:attribute:: log_prob_sum

      Sum of the base-10 log probabilities of its words.

   .. py:method:: set(segmented_string, corrected_string, distance_sum, log_prob_sum)

      Replace all four fields.

   .. py:method:: get_segmented()
                  get_corrected()
                  get_distance()
                  get_probability()

      Return the individual fields. These are equivalent to reading
      the corresponding properties.
