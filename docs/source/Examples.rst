Examples
========

This document contains examples of usage for the SymSpellCppPy library. This library is used for dictionary loading, spelling correction, and error fixing. 

Loading the dictionary
----------------------

.. code-block:: python

    import SymSpellCppPy
    symSpell = SymSpellCppPy.SymSpell()
    symSpell.load_dictionary(corpus="resources/frequency_dictionary_en_82_765.txt", term_index=0, count_index=1, separator=" ")

Checking dictionary properties
------------------------------

The `SymSpell` class provides methods to inspect the loaded dictionary:

- To check the number of words in the dictionary, use the `word_count()` method:

.. code-block:: python

    print(symSpell.word_count())  # Outputs: 82781

- To find the length of the longest word in the dictionary, use the `max_length()` method:

.. code-block:: python

    print(symSpell.max_length())  # Outputs: 28

- To count the number of unique delete combinations formed, use the `entry_count()` method:

.. code-block:: python

    print(symSpell.entry_count())  # Outputs: 661047

Spelling correction
-------------------

The `lookup` method allows you to find the correct spelling for a term from the dictionary:

- To find the closest spelling, use `SymSpellCppPy.Verbosity.CLOSEST`:

.. code-block:: python

    terms = symSpell.lookup("tke", SymSpellCppPy.Verbosity.CLOSEST)
    print(terms[0].term)  # Outputs: "take"

- You can also specify a `max_edit_distance` to limit the search to terms within a certain edit distance:

.. code-block:: python

    terms = symSpell.lookup("extrine", SymSpellCppPy.Verbosity.CLOSEST, max_edit_distance=2)
    print(terms[0].term)  # Outputs: "extreme"

    terms = symSpell.lookup("extrine", SymSpellCppPy.Verbosity.CLOSEST, max_edit_distance=1)
    print(terms)  # Outputs: []

Error fixing
------------

SymSpellCppPy also includes features to fix compound errors and word segmentation issues in sentences:

- To fix compound errors in a sentence, use the `lookup_compound` method:

.. code-block:: python

    terms = symSpell.lookup_compound("whereis th elove hehad dated forImuch of thepast who couqdn'tread in sixthgrade and ins pired him")
    print(terms[0].term)
    # Outputs: "whereas to love head dated for much of theist who couldn't read in sixth grade and inspired him"

- To correct word segmentation issues in a sentence, use the `word_segmentation` method:

.. code-block:: python

    segmented_info = symSpell.word_segmentation("thequickbrownfoxjumpsoverthelazydog")
    print(segmented_info.segmented_string)
    # Outputs: "the quick brown fox jumps over the lazy dog"

    segmented_info = symSpell.word_segmentation("thequickbrownfoxjumpsoverthelazydog")
    print(segmented_info.corrected_string)
    # Outputs: "they quick brown fox jumps over therapy dog"

Saving and Loading SymSpell object
----------------------------------

To save the internal representation of a loaded `SymSpell` for fast reuse next time, use the `save_pickle` method. Do not use pickle natively:

.. code-block:: python

    symSpell.save_pickle("symspell_binary.bin")

To load the internal representation of a loaded `SymSpell` from a saved binary, use the `load_pickle` method:

.. code-block:: python

    anotherSymSpell = SymSpellCppPy.SymSpell()
    anotherSymSpell.load_pickle("symspell_binary.bin")
    terms = anotherSymSpell.lookup("tke", SymSpellCppPy.Verbosity.CLOSEST)
    print(terms[0].term)

Top N suggestions
-------------------

You can also request the top N suggestions for a given word:

.. code-block:: python

    # To get the top 5 closest terms to a given word, use the `TOP` verbosity:
    terms = symSpell.lookup("huse", SymSpellCppPy.Verbosity.TOP, max_edit_distance=2, include_unknown=True)
    for term in terms[:5]:
        print(term.term)
    # Outputs: "house", "use", "hue", "hues", "hose"

Ignoring case and digits
------------------------

By default, SymSpellCppPy is case-sensitive and considers digits as valid characters. However, you can modify this behavior:

.. code-block:: python

    # To ignore case when checking a term, use the `ignore_case` parameter:
    terms = symSpell.lookup("THe", SymSpellCppPy.Verbosity.CLOSEST, ignore_case=True)
    print(terms[0].term)  # Outputs: "the"

    # To ignore digits when checking a term, use the `ignore_digit` parameter:
    terms = symSpell.lookup("3rd", SymSpellCppPy.Verbosity.CLOSEST, ignore_digit=True)
    print(terms[0].term)  # Outputs: "red"

Ignoring words with numbers
----------------------------

You may also choose to ignore words containing numbers:

.. code-block:: python

    # To ignore words with numbers when checking a term, use the `ignore_word_with_number` parameter:
    terms = symSpell.lookup("l33t", SymSpellCppPy.Verbosity.CLOSEST, ignore_word_with_number=True)
    print(terms[0].term)  # Outputs: "let"
