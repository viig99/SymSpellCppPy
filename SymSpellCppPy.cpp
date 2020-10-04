//
// Created by vigi99 on 27/09/20.
//

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "library.h"

namespace py = pybind11;

PYBIND11_MODULE(SymSpellCppPy, m) {
    m.doc() = R"pbdoc(
        Pybind11 binding for SymSpellPy
        -------------------------------
        .. currentmodule:: SymSpellCppPy
        .. autosummary::
           :toctree: _generate
           symspell
    )pbdoc";

    py::class_<symspellcpppy::Info>(m, "Info")
            .def(py::init<>())
            .def("set", &symspellcpppy::Info::set, "Set Info properties", py::arg("segmented_string"),
                 py::arg("corrected_string"),
                 py::arg("distance_sum"), py::arg("log_prob_sum"))
            .def("get_segmented", &symspellcpppy::Info::getSegmented, "The word segmented string.")
            .def("get_corrected", &symspellcpppy::Info::getCorrected,
                 "The word segmented and spelling corrected string.")
            .def("get_distance", &symspellcpppy::Info::getDistance,
                 "The Edit distance sum between input string and corrected string.")
            .def("get_probability", &symspellcpppy::Info::getProbability,
                 "The Sum of word occurrence probabilities in log scale (a measure of how common and probable the corrected segmentation is).")
            .def_property_readonly("segmented_string", &symspellcpppy::Info::getSegmented, "The word segmented string.")
            .def_property_readonly("corrected_string", &symspellcpppy::Info::getCorrected,
                                   "The word segmented and spelling corrected string.")
            .def_property_readonly("distance_sum", &symspellcpppy::Info::getDistance,
                                   "The Edit distance sum between input string and corrected string.")
            .def_property_readonly("log_prob_sum", &symspellcpppy::Info::getProbability,
                                   "The Sum of word occurrence probabilities in log scale (a measure of how common and probable the corrected segmentation is).")
            .def("__repr__",
                 [](const symspellcpppy::Info &a) {
                     return "<Info corrected string ='" + a.getCorrected() + "'>";
                 }
            );

    py::class_<SuggestItem>(m, "SuggestItem")
            .def(py::init<xstring, int, int64_t>())
            .def("__eq__", [](const SuggestItem &a, const SuggestItem &b) {
                return a.Equals(b);
            }, "Compare ==")
            .def("__lt__", [](const SuggestItem &a, const SuggestItem &b) {
                return SuggestItem::compare(a, b);
            }, "Order by distance ascending, then by frequency count descending.")
            .def("__repr__",
                 [](const SuggestItem &a) {
                     return a.term + ", " + std::to_string(a.distance) + ", " + std::to_string(a.count);
                 }
            )
            .def("__str__",
                 [](const SuggestItem &a) {
                     return a.term + ", " + std::to_string(a.distance) + ", " + std::to_string(a.count);
                 }
            )
            .def_readwrite("term", &SuggestItem::term, "The suggested correctly spelled word.")
            .def_readwrite("distance", &SuggestItem::distance,
                           "Edit distance between searched for word and suggestion.")
            .def_readwrite("count", &SuggestItem::count,
                           "Frequency of suggestion in the dictionary (a measure of how common the word is).");

    py::enum_<symspellcpppy::Verbosity>(m, "Verbosity")
            .value("TOP", symspellcpppy::Verbosity::Top,
                   "The suggestion with the highest term frequency of the suggestions of smallest edit distance found.")
            .value("CLOSEST", symspellcpppy::Verbosity::Closest,
                   "All suggestions of smallest edit distance found, the suggestions are ordered by term frequency.")
            .value("ALL", symspellcpppy::Verbosity::All,
                   "All suggestions <= maxEditDistance, the suggestions are ordered by edit distance, then by term frequency (slower, no early termination).")
            .export_values();

    py::class_<symspellcpppy::SymSpell>(m, "SymSpell")
            .def(py::init<int, int, int, int, unsigned char>(), "SymSpell builder options",
                 py::arg("max_dictionary_edit_distance") = DEFAULT_MAX_EDIT_DISTANCE,
                 py::arg("prefix_length") = DEFAULT_PREFIX_LENGTH,
                 py::arg("count_threshold") = DEFAULT_COUNT_THRESHOLD,
                 py::arg("initial_capacity") = DEFAULT_INITIAL_CAPACITY,
                 py::arg("compact_level") = DEFAULT_COMPACT_LEVEL
            )
            .def("word_count", &symspellcpppy::SymSpell::WordCount, "Number of words entered.")
            .def("max_length", &symspellcpppy::SymSpell::MaxLength, "Max length of words entered.")
            .def("entry_count", &symspellcpppy::SymSpell::EntryCount, "Total number of deletes formed.")
            .def("count_threshold", &symspellcpppy::SymSpell::CountThreshold,
                 "Frequency of word so that its considered a valid word for spelling correction.")
            .def("create_dictionary_entry", [](symspellcpppy::SymSpell &sym, const xstring &key, int64_t count) {
                auto staging = std::make_shared<SuggestionStage>(128);
                sym.CreateDictionaryEntry(Helpers::string_lower(key), count, staging);
                sym.CommitStaged(staging);
                return sym.EntryCount() > 0;
            }, "Create/Update an entry in the dictionary.", py::arg("key"), py::arg("count"))
            .def("delete_dictionary_entry", &symspellcpppy::SymSpell::DeleteDictionaryEntry,
                 "Delete the key from the dictionary & updates internal representation accordingly.",
                 py::arg("key"))
            .def("load_bigram_dictionary", py::overload_cast<const std::string &, int, int, xchar>(
                    &symspellcpppy::SymSpell::LoadBigramDictionary),
                 "Load multiple dictionary entries from a file of word/frequency count pairs.",
                 py::arg("corpus"),
                 py::arg("term_index"),
                 py::arg("count_index"),
                 py::arg("separator") = DEFAULT_SEPARATOR_CHAR)
            .def("load_dictionary", py::overload_cast<const std::string &, int, int, xchar>(
                    &symspellcpppy::SymSpell::LoadDictionary),
                 "Load multiple dictionary entries from a file of word/frequency count pairs.",
                 py::arg("corpus"),
                 py::arg("term_index"),
                 py::arg("count_index"),
                 py::arg("separator") = DEFAULT_SEPARATOR_CHAR)
            .def("create_dictionary", py::overload_cast<const std::string &>(
                    &symspellcpppy::SymSpell::CreateDictionary),
                 "Load multiple dictionary words from a file containing plain text.",
                 py::arg("corpus"))
            .def("purge_below_threshold_words", &symspellcpppy::SymSpell::PurgeBelowThresholdWords,
                 "Remove all below threshold words from the dictionary.")
            .def("lookup", py::overload_cast<xstring, symspellcpppy::Verbosity>(
                    &symspellcpppy::SymSpell::Lookup),
                 " Find suggested spellings for a given input word, using the maximum\n"
                 "    edit distance specified during construction of the SymSpell dictionary.",
                 py::arg("input"),
                 py::arg("verbosity"))
            .def("lookup", py::overload_cast<xstring, symspellcpppy::Verbosity, int>(
                    &symspellcpppy::SymSpell::Lookup),
                 " Find suggested spellings for a given input word, using the maximum\n"
                 "    edit distance provided to the function.",
                 py::arg("input"),
                 py::arg("verbosity"),
                 py::arg("max_edit_distance"))
            .def("lookup", py::overload_cast<xstring, symspellcpppy::Verbosity, int, bool>(
                    &symspellcpppy::SymSpell::Lookup),
                 " Find suggested spellings for a given input word, using the maximum\n"
                 "    edit distance provided to the function and include input word in suggestions, if no words within edit distance found.",
                 py::arg("input"),
                 py::arg("verbosity"),
                 py::arg("max_edit_distance"),
                 py::arg("include_unknown"))
            .def("lookup", py::overload_cast<xstring, symspellcpppy::Verbosity, int, bool, bool>(
                    &symspellcpppy::SymSpell::Lookup),
                 " Find suggested spellings for a given input word, using the maximum\n"
                 "    edit distance provided to the function and include input word in suggestions, if no words within edit distance found & preserve transfer casing.",
                 py::arg("input"),
                 py::arg("verbosity"),
                 py::arg("max_edit_distance") = DEFAULT_MAX_EDIT_DISTANCE,
                 py::arg("include_unknown") = false,
                 py::arg("transfer_casing") = false)
            .def("lookup_compound", py::overload_cast<const xstring &>(
                    &symspellcpppy::SymSpell::LookupCompound),
                 " LookupCompound supports compound aware automatic spelling correction of multi-word input strings with three cases:\n"
                 "    1. mistakenly inserted space into a correct word led to two incorrect terms \n"
                 "    2. mistakenly omitted space between two correct words led to one incorrect combined term\n"
                 "    3. multiple independent input terms with/without spelling errors",
                 py::arg("input"))
            .def("lookup_compound", py::overload_cast<const xstring &, int>(
                    &symspellcpppy::SymSpell::LookupCompound),
                 " LookupCompound supports compound aware automatic spelling correction of multi-word input strings with three cases:\n"
                 "    1. mistakenly inserted space into a correct word led to two incorrect terms \n"
                 "    2. mistakenly omitted space between two correct words led to one incorrect combined term\n"
                 "    3. multiple independent input terms with/without spelling errors",
                 py::arg("input"),
                 py::arg("max_edit_distance"))
            .def("lookup_compound", py::overload_cast<const xstring &, int, bool>(
                    &symspellcpppy::SymSpell::LookupCompound),
                 " LookupCompound supports compound aware automatic spelling correction of multi-word input strings with three cases:\n"
                 "    1. mistakenly inserted space into a correct word led to two incorrect terms \n"
                 "    2. mistakenly omitted space between two correct words led to one incorrect combined term\n"
                 "    3. multiple independent input terms with/without spelling errors",
                 py::arg("input"),
                 py::arg("max_edit_distance"),
                 py::arg("transfer_casing"))
            .def("word_segmentation", py::overload_cast<const xstring &>(
                    &symspellcpppy::SymSpell::WordSegmentation),
                 " WordSegmentation divides a string into words by inserting missing spaces at the appropriate positions\n"
                 "    misspelled words are corrected and do not affect segmentation\n"
                 "    existing spaces are allowed and considered for optimum segmentation",
                 py::arg("input"))
            .def("word_segmentation", py::overload_cast<const xstring &, int>(
                    &symspellcpppy::SymSpell::WordSegmentation),
                 " WordSegmentation divides a string into words by inserting missing spaces at the appropriate positions\n"
                 "    misspelled words are corrected and do not affect segmentation\n"
                 "    existing spaces are allowed and considered for optimum segmentation",
                 py::arg("input"),
                 py::arg("max_edit_distance"))
            .def("word_segmentation", py::overload_cast<const xstring &, int, int>(
                    &symspellcpppy::SymSpell::WordSegmentation),
                 " WordSegmentation divides a string into words by inserting missing spaces at the appropriate positions\n"
                 "    misspelled words are corrected and do not affect segmentation\n"
                 "    existing spaces are allowed and considered for optimum segmentation",
                 py::arg("input"),
                 py::arg("max_edit_distance"),
                 py::arg("max_segmentation_word_length"))
            .def("save_pickle", [](symspellcpppy::SymSpell &sym, const std::string &filepath) {
                     std::ofstream binary_path(filepath, std::ios::out | std::ios::app | std::ios::binary);
                     if (binary_path.is_open()) {
                         cereal::BinaryOutputArchive ar(binary_path);
                         ar(sym);
                     } else {
                         throw std::invalid_argument("Cannot save to file: " + filepath);
                     }
                 }, "Save internal representation to file",
                 py::arg("filepath"))
            .def("load_pickle", [](symspellcpppy::SymSpell &sym, const std::string &filepath) {
                     if (Helpers::file_exists(filepath)) {
                         std::ifstream binary_path(filepath, std::ios::binary);
                         cereal::BinaryInputArchive ar(binary_path);
                         ar(sym);
                     } else {
                         throw std::invalid_argument("Unable to load file from filepath: " + filepath);
                     }
                 }, "Load internal representation from file",
                 py::arg("filepath"));

}