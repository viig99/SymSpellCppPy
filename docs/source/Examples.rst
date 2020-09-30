Examples
========

To load the dictionary use::

    import SymSpellCppPy
    symSpell = SymSpellCppPy.SymSpell()
    symSpell.load_dictionary(corpus="resources/frequency_dictionary_en_82_765.txt", term_index=0, count_index=1, separator=" ")

To check number of words in dictionary::

    print(symSpell.word_count())
    >> 82781

To check max word length::

    print(symSpell.max_length())
    >> 28

To check number of delete combinations formed::

    print(symSpell.entry_count())
    >> 661047

To lookup and find the correct spelling for a term from the dictonary use::

    terms = symSpell.lookup("tke", SymSpellCppPy.Verbosity.CLOSEST)
    print(terms[0].term)
    >> "take"

To lookup and find the correct spelling for a term from the dictonary within a certain edit-distance use::


    terms = symSpell.lookup("extrine", SymSpellCppPy.Verbosity.CLOSEST, max_edit_distance=2)
    print(terms[0].term)
    >> "extreme"

    terms = symSpell.lookup("extrine", SymSpellCppPy.Verbosity.CLOSEST, max_edit_distance=1)
    print(terms)
    >> []

To fix compound errors in a sentence use::

    terms = symSpell.lookup_compound("whereis th elove hehad dated forImuch of thepast who couqdn'tread in sixthgrade and ins pired him")
    print(terms[0].term)
    >> "whereas to love head dated for much of theist who couldn't read in sixth grade and inspired him"

To fix word segmentation issues in a sentence use::

    segmented_info = symSpell.word_segmentation("thequickbrownfoxjumpsoverthelazydog")
    print(segmented_info.segmented_string)
    >> "the quick brown fox jumps over the lazy dog"

    segmented_info = symSpell.word_segmentation("thequickbrownfoxjumpsoverthelazydog")
    print(segmented_info.corrected_string)
    >> "they quick brown fox jumps over therapy dog"

To save the internal representation of loaded SymSpell for fast reuse next time, dont use pickle natively, use this function instead::

    symSpell.save_pickle("symspell_binary.bin")

To load the internal representation of loaded SymSpell from a saved binary use::

    anotherSymSpell = SymSpellCppPy.SymSpell()
    anotherSymSpell.load_pickle("symspell_binary.bin")
    terms = anotherSymSpell.lookup("tke", SymSpellCppPy.Verbosity.CLOSEST)
    print(terms[0].term)

