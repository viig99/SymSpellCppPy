SymSpellCppPy Docs
==================

Examples
========

To load the dictionary use::

    import SymSpellCppPy
    symSpell = SymSpellCppPy.SymSpell()
    symSpell.load_dictionary(corpus="resources/frequency_dictionary_en_82_765.txt", term_index=0, count_index=1, sep=" ")

To lookup and find the correct spelling for a term from the dictonary use::

    terms = symSpell.lookup_term("tke", SymSpellCppPy.Verbosity.Closest)
    print(terms[0])
    >> "take"

To lookup and find the correct spelling for a term from the dictonary within a certain edit-distance use::


    terms = symSpell.lookup_term("extrine", SymSpellCppPy.Verbosity.Closest, max_edit_distance=2)
    print(terms[0])
    >> "extreme"

    terms = symSpell.lookup_term("extrine", SymSpellCppPy.Verbosity.Closest, max_edit_distance=1)
    print(terms)
    >> []

To fix compound errors in a sentence use::

    terms = symSpell.lookup_compound_term("whereis th elove hehad dated forImuch of thepast who couqdn'tread in sixthgrade and ins pired him")
    print(terms[0])
    >> "whereas to love head dated for much of theist who couldn't read in sixth grade and inspired him"

To fix word segmentation issues in a sentence use::

    segmented_info = symSpell.word_segmentation("thequickbrownfoxjumpsoverthelazydog")
    print(segmented_info.get_segmented())
    >> "the quick brown fox jumps over the lazy dog"

    segmented_info = symSpell.word_segmentation("thequickbrownfoxjumpsoverthelazydog")
    print(segmented_info.get_corrected())
    >> "they quick brown fox jumps over therapy dog"


.. toctree::
   :maxdepth: 2


Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
