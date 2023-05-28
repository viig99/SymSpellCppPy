//
// Created by vigi99 on 27/09/20.
//

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "library.h"

namespace py = pybind11;

PYBIND11_MODULE(SymSpellCppPy, m)
{
     m.doc() = R"pbdoc(
        SymSpellCppPy: Pybind11 binding for SymSpellPy
        ----------------------------------------------
        .. currentmodule:: SymSpellCppPy
        .. autosummary::
           :toctree: _generate

           Info
           SuggestItem
           Verbosity
           SymSpell
    )pbdoc";

     py::class_<symspellcpppy::Info>(m, "Info")
         .def(py::init<>(), R"pbdoc(
            Constructor of Info class.
        )pbdoc")
         .def("set", &symspellcpppy::Info::set, R"pbdoc(
            Set the properties of Info object.

            :param segmented_string: Word segmented string.
            :param corrected_string: Word segmented and spelling corrected string.
            :param distance_sum: Edit distance sum between input string and corrected string.
            :param log_prob_sum: Sum of word occurrence probabilities in log scale (a measure of how common and probable the corrected segmentation is).
        )pbdoc",
              py::arg("segmented_string"), py::arg("corrected_string"), py::arg("distance_sum"), py::arg("log_prob_sum"))
         .def("get_segmented", &symspellcpppy::Info::getSegmented, R"pbdoc(
            Get the word segmented string.
        )pbdoc")
         .def("get_corrected", &symspellcpppy::Info::getCorrected, R"pbdoc(
            Get the word segmented and spelling corrected string.
        )pbdoc")
         .def("get_distance", &symspellcpppy::Info::getDistance, R"pbdoc(
            Get the edit distance sum between input string and corrected string.
        )pbdoc")
         .def("get_probability", &symspellcpppy::Info::getProbability, R"pbdoc(
            Get the sum of word occurrence probabilities in log scale. 
            This is a measure of how common and probable the corrected segmentation is.
        )pbdoc")
         .def_property_readonly("segmented_string", &symspellcpppy::Info::getSegmented, R"pbdoc(
            Read-only property to get the word segmented string.
        )pbdoc")
         .def_property_readonly("corrected_string", &symspellcpppy::Info::getCorrected, R"pbdoc(
            Read-only property to get the word segmented and spelling corrected string.
        )pbdoc")
         .def_property_readonly("distance_sum", &symspellcpppy::Info::getDistance, R"pbdoc(
            Read-only property to get the edit distance sum between input string and corrected string.
        )pbdoc")
         .def_property_readonly("log_prob_sum", &symspellcpppy::Info::getProbability, R"pbdoc(
            Read-only property to get the sum of word occurrence probabilities in log scale. 
            This is a measure of how common and probable the corrected segmentation is.
        )pbdoc")
         .def("__repr__",
              [](const symspellcpppy::Info &a)
              {
                   return "<Info corrected_string ='" + a.getCorrected() + "'>";
              });

     py::class_<SuggestItem>(m, "SuggestItem", R"pbdoc(
        SuggestItem is a class that contains a suggested correct spelling for a misspelled word.
    )pbdoc")
         .def(py::init<xstring, int, int64_t>(), R"pbdoc(
        Initializes a new instance of the SuggestItem class.
    )pbdoc")
         .def(
             "__eq__", [](const SuggestItem &a, const SuggestItem &b)
             { return a.Equals(b); },
             R"pbdoc(
            Compares the current object with another object of the same type.
    )pbdoc")
         .def(
             "__lt__", [](const SuggestItem &a, const SuggestItem &b)
             { return SuggestItem::compare(a, b); },
             R"pbdoc(
            Orders the objects of the same type.
    )pbdoc")
         .def(
             "__repr__",
             [](const SuggestItem &a)
             {
                  return a.term + ", " + std::to_string(a.distance) + ", " + std::to_string(a.count);
             },
             R"pbdoc(
            Returns a string that represents the current object.
    )pbdoc")
         .def(
             "__str__",
             [](const SuggestItem &a)
             {
                  return a.term + ", " + std::to_string(a.distance) + ", " + std::to_string(a.count);
             },
             R"pbdoc(
            Returns a string that represents the current object.
    )pbdoc")
         .def_readwrite("term", &SuggestItem::term, R"pbdoc(
        Gets or sets the suggested correctly spelled word.
    )pbdoc")
         .def_readwrite("distance", &SuggestItem::distance, R"pbdoc(
        Gets or sets the edit distance between the searched for word and the suggestion.
    )pbdoc")
         .def_readwrite("count", &SuggestItem::count, R"pbdoc(
        Gets or sets the frequency of the suggestion in the dictionary (a measure of how common the word is).
    )pbdoc");

     py::enum_<symspellcpppy::Verbosity>(m, "Verbosity")
         .value("TOP", symspellcpppy::Verbosity::Top, R"pbdoc(
          Top suggestion with the highest term frequency of the suggestions of smallest edit distance found.
     )pbdoc")
         .value("CLOSEST", symspellcpppy::Verbosity::Closest, R"pbdoc(
          All suggestions of smallest edit distance found, the suggestions are ordered by term frequency.
     )pbdoc")
         .value("ALL", symspellcpppy::Verbosity::All, R"pbdoc(
          All suggestions <= maxEditDistance, the suggestions are ordered by edit distance, then by term frequency (highest first)
     )pbdoc")
         .export_values();

     py::class_<symspellcpppy::SymSpell>(m, "SymSpell", R"pbdoc(
        SymSpell is a class that provides fast and accurate spelling correction using Symmetric Delete spelling correction algorithm.
    )pbdoc")
         .def(py::init<int, int, int, int, unsigned char>(), "SymSpell builder options",
              py::arg("max_dictionary_edit_distance") = DEFAULT_MAX_EDIT_DISTANCE,
              py::arg("prefix_length") = DEFAULT_PREFIX_LENGTH,
              py::arg("count_threshold") = DEFAULT_COUNT_THRESHOLD,
              py::arg("initial_capacity") = DEFAULT_INITIAL_CAPACITY,
              py::arg("compact_level") = DEFAULT_COMPACT_LEVEL)
         .def("word_count", &symspellcpppy::SymSpell::WordCount, R"pbdoc(
        Retrieves the total number of words in the dictionary.
    )pbdoc")
         .def("max_length", &symspellcpppy::SymSpell::MaxLength, R"pbdoc(
        Retrieves the maximum length of words in the dictionary.
    )pbdoc")
         .def("entry_count", &symspellcpppy::SymSpell::EntryCount, R"pbdoc(
        Retrieves the total number of delete words formed in the dictionary.
    )pbdoc")
         .def("count_threshold", &symspellcpppy::SymSpell::CountThreshold, R"pbdoc(
        Retrieves the frequency threshold to be considered as a valid word for spelling correction.
    )pbdoc")
         .def(
             "create_dictionary_entry", [](symspellcpppy::SymSpell &sym, const xstring &key, int64_t count)
             {
          auto staging = std::make_shared<SuggestionStage>(128);
          sym.CreateDictionaryEntry(Helpers::string_lower(key), count, staging);
          sym.CommitStaged(staging);
          return sym.EntryCount() > 0; },
             R"pbdoc(
                Create or update an entry in the dictionary.
    )pbdoc",
             py::arg("key"), py::arg("count"))
         .def("delete_dictionary_entry", &symspellcpppy::SymSpell::DeleteDictionaryEntry, R"pbdoc(
        Deletes a word from the dictionary and updates internal representation accordingly.
    )pbdoc",
              py::arg("key"))
         .def("load_bigram_dictionary", py::overload_cast<const std::string &, int, int, xchar>(&symspellcpppy::SymSpell::LoadBigramDictionary), R"pbdoc(
        Load multiple dictionary entries from a file of word/frequency count pairs.
    )pbdoc",
              py::arg("corpus"), py::arg("term_index"), py::arg("count_index"), py::arg("separator") = DEFAULT_SEPARATOR_CHAR)
         .def("load_dictionary", py::overload_cast<const std::string &, int, int, xchar>(&symspellcpppy::SymSpell::LoadDictionary), R"pbdoc(
        Load multiple dictionary entries from a file of word/frequency count pairs.
    )pbdoc",
              py::arg("corpus"), py::arg("term_index"), py::arg("count_index"), py::arg("separator") = DEFAULT_SEPARATOR_CHAR)
         .def("create_dictionary", py::overload_cast<const std::string &>(&symspellcpppy::SymSpell::CreateDictionary), R"pbdoc(
        Load multiple dictionary words from a file containing plain text.
    )pbdoc",
              py::arg("corpus"))
         .def("purge_below_threshold_words", &symspellcpppy::SymSpell::PurgeBelowThresholdWords,
              "Remove all below threshold words from the dictionary.")
         .def("lookup", py::overload_cast<const xstring &, symspellcpppy::Verbosity>(&symspellcpppy::SymSpell::Lookup), R"pbdoc(
        Find suggested spellings for a given input word, using the maximum
        edit distance specified during construction of the SymSpell dictionary.
     )pbdoc",
              py::arg("input"),
              py::arg("verbosity"))
         .def("lookup", py::overload_cast<const xstring &, symspellcpppy::Verbosity, int>(&symspellcpppy::SymSpell::Lookup), R"pbdoc(
        Find suggested spellings for a given input word, using the maximum
        edit distance provided to the function.
     )pbdoc",
              py::arg("input"),
              py::arg("verbosity"),
              py::arg("max_edit_distance"))
         .def("lookup", py::overload_cast<const xstring &, symspellcpppy::Verbosity, int, bool>(&symspellcpppy::SymSpell::Lookup), R"pbdoc(
        Find suggested spellings for a given input word, using the maximum\
        edit distance provided to the function and include input word in suggestions if no words within edit distance found.
     )pbdoc",
              py::arg("input"),
              py::arg("verbosity"),
              py::arg("max_edit_distance"),
              py::arg("include_unknown"))
         .def("lookup", py::overload_cast<const xstring &, symspellcpppy::Verbosity, int, bool, bool>(&symspellcpppy::SymSpell::Lookup), R"pbdoc(
        Find suggested spellings for a given input word, using the maximum
        edit distance provided to the function and include input word in suggestions if no words within edit distance found & preserve transfer casing.
     )pbdoc",
              py::arg("input"),
              py::arg("verbosity"),
              py::arg("max_edit_distance") = DEFAULT_MAX_EDIT_DISTANCE,
              py::arg("include_unknown") = false,
              py::arg("transfer_casing") = false)
         .def("lookup_compound", py::overload_cast<const xstring &>(&symspellcpppy::SymSpell::LookupCompound),
              R"pbdoc(
        LookupCompound supports compound-aware automatic spelling correction of multi-word input strings with three cases:
          1. Mistakenly inserted space into a correct word led to two incorrect terms.
          2. Mistakenly omitted space between two correct words led to one incorrect combined term.
          3. Multiple independent input terms with/without spelling errors.
    )pbdoc",
              py::arg("input"))
         .def("lookup_compound", py::overload_cast<const xstring &, int>(&symspellcpppy::SymSpell::LookupCompound),
              R"pbdoc(
        LookupCompound supports compound-aware automatic spelling correction of multi-word input strings with three cases:
          1. Mistakenly inserted space into a correct word led to two incorrect terms.
          2. Mistakenly omitted space between two correct words led to one incorrect combined term.
          3. Multiple independent input terms with/without spelling errors.
    )pbdoc",
              py::arg("input"),
              py::arg("max_edit_distance"))
         .def("lookup_compound", py::overload_cast<const xstring &, int, bool>(&symspellcpppy::SymSpell::LookupCompound),
              R"pbdoc(
        LookupCompound supports compound-aware automatic spelling correction of multi-word input strings with three cases:
          1. Mistakenly inserted space into a correct word led to two incorrect terms.
          2. Mistakenly omitted space between two correct words led to one incorrect combined term.
          3. Multiple independent input terms with/without spelling errors.
    )pbdoc",
              py::arg("input"),
              py::arg("max_edit_distance"),
              py::arg("transfer_casing"))
         .def("word_segmentation", py::overload_cast<const xstring &>(&symspellcpppy::SymSpell::WordSegmentation),
              R"pbdoc(
        WordSegmentation divides a string into words by inserting missing spaces at the appropriate positions.
        Misspelled words are corrected and do not affect segmentation.
        Existing spaces are allowed and considered for optimum segmentation.
    )pbdoc",
              py::arg("input"))
         .def("word_segmentation", py::overload_cast<const xstring &, int>(&symspellcpppy::SymSpell::WordSegmentation),
              R"pbdoc(
        WordSegmentation divides a string into words by inserting missing spaces at the appropriate positions.
        Misspelled words are corrected and do not affect segmentation.
        Existing spaces are allowed and considered for optimum segmentation.
    )pbdoc",
              py::arg("input"),
              py::arg("max_edit_distance"))
         .def("word_segmentation", py::overload_cast<const xstring &, int, int>(&symspellcpppy::SymSpell::WordSegmentation),
              R"pbdoc(
        WordSegmentation divides a string into words by inserting missing spaces at the appropriate positions.
        Misspelled words are corrected and do not affect segmentation.
        Existing spaces are allowed and considered for optimum segmentation.
    )pbdoc",
              py::arg("input"),
              py::arg("max_edit_distance"),
              py::arg("max_segmentation_word_length"))
         .def(
             "save_pickle", [](symspellcpppy::SymSpell &sym, const std::string &filepath)
             {
                     std::ofstream binary_path(filepath, std::ios::out | std::ios::app | std::ios::binary);
                     if (binary_path.is_open()) {
                         cereal::BinaryOutputArchive ar(binary_path);
                         ar(sym);
                     } else {
                         throw std::invalid_argument("Cannot save to file: " + filepath);
                     } },
             "Save internal representation to file",
             py::arg("filepath"))
         .def(
             "load_pickle", [](symspellcpppy::SymSpell &sym, const std::string &filepath)
             {
                     if (Helpers::file_exists(filepath)) {
                         std::ifstream binary_path(filepath, std::ios::binary);
                         cereal::BinaryInputArchive ar(binary_path);
                         ar(sym);
                     } else {
                         throw std::invalid_argument("Unable to load file from filepath: " + filepath);
                     } },
             "Load internal representation from file",
             py::arg("filepath"))
         .def(
             "save_pickle_bytes", [](symspellcpppy::SymSpell &sym)
             {
                    std::ostringstream binary_stream(std::ios::out | std::ios::binary);
                    cereal::BinaryOutputArchive ar(binary_stream);
                    ar(sym);

                    return py::bytes(binary_stream.str()); },
             "Save internal representation to bytes")
         .def(
             "load_pickle_bytes", [](symspellcpppy::SymSpell &sym, py::buffer bytes)
             {
                    py::buffer_info buff = bytes.request();

                    if (buff.strides.size() != 1) {
                         throw std::domain_error("Unable to load buffer: buffer should be 1-dimensional.");
                    }

                    if (buff.strides[0] != buff.itemsize) {
                         throw std::domain_error("Unable to load buffer: buffer should be contiguous.");
                    }

                    std::string const bytes_str(reinterpret_cast<char*>(buff.ptr), buff.size * buff.itemsize);
                    std::istringstream binary_stream(bytes_str, std::ios::in | std::ios::binary);

                    cereal::BinaryInputArchive ar(binary_stream);
                    ar(sym); },
             "Load internal representation from buffers, such as 'bytes' and 'memoryview'",
             py::arg("bytes"));
}
