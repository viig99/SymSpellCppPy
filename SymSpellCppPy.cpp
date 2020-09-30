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
            .def("get_segmented", &symspellcpppy::Info::getSegmented)
            .def("get_corrected", &symspellcpppy::Info::getCorrected)
            .def("get_distance", &symspellcpppy::Info::getDistance)
            .def("get_probability", &symspellcpppy::Info::getProbability)
            .def_property_readonly("segmented_string", &symspellcpppy::Info::getSegmented)
            .def_property_readonly("corrected_string", &symspellcpppy::Info::getCorrected)
            .def_property_readonly("distance_sum", &symspellcpppy::Info::getDistance)
            .def_property_readonly("log_prob_sum", &symspellcpppy::Info::getProbability)
            .def("__repr__",
                 [](const symspellcpppy::Info &a) {
                     return "<Info corrected string ='" + a.getCorrected() + "'>";
                 }
            );

    py::class_<SuggestItem>(m, "SuggestItem")
            .def(py::init<xstring, int, int64_t>())
            .def("__eq__", [](const SuggestItem &a, const SuggestItem &b) {
                return a.Equals(b);
            })
            .def("__lt__", [](const SuggestItem &a, const SuggestItem &b) {
                return SuggestItem::compare(a, b);
            })
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
            .def_readwrite("term", &SuggestItem::term)
            .def_readwrite("distance", &SuggestItem::distance)
            .def_readwrite("count", &SuggestItem::count);

    py::enum_<symspellcpppy::Verbosity>(m, "Verbosity")
            .value("TOP", symspellcpppy::Verbosity::Top, "Get the top-k matches")
            .value("CLOSEST", symspellcpppy::Verbosity::Closest, "Get the closest match")
            .value("ALL", symspellcpppy::Verbosity::All, "Get all matches ranked on basis of edit distance.")
            .export_values();

    py::class_<symspellcpppy::SymSpell>(m, "SymSpell")
            .def(py::init<int, int, int, int, unsigned char>(), "SymSpell builder options",
                 py::arg("initial_capacity") = DEFAULT_INITIAL_CAPACITY,
                 py::arg("max_dictionary_edit_distance") = DEFAULT_MAX_EDIT_DISTANCE,
                 py::arg("prefix_length") = DEFAULT_PREFIX_LENGTH,
                 py::arg("count_threshold") = DEFAULT_COUNT_THRESHOLD,
                 py::arg("compact_level") = DEFAULT_COMPACT_LEVEL
            )
            .def("word_count", &symspellcpppy::SymSpell::WordCount, "Number of words entered.")
            .def("max_length", &symspellcpppy::SymSpell::MaxLength, "Max length of words entered.")
            .def("entry_count", &symspellcpppy::SymSpell::EntryCount, "Total number of deletes formed.")
            .def("count_threshold", &symspellcpppy::SymSpell::CountThreshold, "Count threshold.")
            .def("load_bigram_dictionary", py::overload_cast<const std::string &, int, int, xchar>(
                    &symspellcpppy::SymSpell::LoadBigramDictionary),
                 "Load the bi-gram dictionary",
                 py::arg("corpus"),
                 py::arg("term_index"),
                 py::arg("count_index"),
                 py::arg("separator") = DEFAULT_SEPARATOR_CHAR)
            .def("load_dictionary", py::overload_cast<const std::string &, int, int, xchar>(
                    &symspellcpppy::SymSpell::LoadDictionary),
                 "Load the dictionary",
                 py::arg("corpus"),
                 py::arg("term_index"),
                 py::arg("count_index"),
                 py::arg("separator") = DEFAULT_SEPARATOR_CHAR)
            .def("create_dictionary", py::overload_cast<const std::string &>(
                    &symspellcpppy::SymSpell::CreateDictionary),
                 "create dictionary",
                 py::arg("corpus"))
            .def("purge_below_threshold_words", &symspellcpppy::SymSpell::PurgeBelowThresholdWords,
                 "purge below threshold words")
            .def("lookup", py::overload_cast<xstring, symspellcpppy::Verbosity>(
                    &symspellcpppy::SymSpell::Lookup),
                 "lookup word in dictionary",
                 py::arg("input"),
                 py::arg("verbosity"))
            .def("lookup", py::overload_cast<xstring, symspellcpppy::Verbosity, int>(
                    &symspellcpppy::SymSpell::Lookup),
                 "lookup word in dictionary",
                 py::arg("input"),
                 py::arg("verbosity"),
                 py::arg("max_edit_distance"))
            .def("lookup", py::overload_cast<xstring, symspellcpppy::Verbosity, int, bool>(
                    &symspellcpppy::SymSpell::Lookup),
                 "lookup word in dictionary",
                 py::arg("input"),
                 py::arg("verbosity"),
                 py::arg("max_edit_distance"),
                 py::arg("include_unknown"))
            .def("lookup_compound", py::overload_cast<const xstring &, int>(
                    &symspellcpppy::SymSpell::LookupCompound),
                 "lookup compound words from the dictionary",
                 py::arg("input"),
                 py::arg("max_edit_distance"))
            .def("lookup_compound", py::overload_cast<const xstring &>(
                    &symspellcpppy::SymSpell::LookupCompound),
                 "lookup compound words from the dictionary",
                 py::arg("input"))
            .def("word_segmentation", py::overload_cast<const xstring &>(
                    &symspellcpppy::SymSpell::WordSegmentation),
                 "insert spaces in between words in a sentence",
                 py::arg("input"))
            .def("word_segmentation", py::overload_cast<const xstring &, int>(
                    &symspellcpppy::SymSpell::WordSegmentation),
                 "insert spaces in between words in a sentence",
                 py::arg("input"),
                 py::arg("max_edit_distance"))
            .def("word_segmentation", py::overload_cast<const xstring &, int, int>(
                    &symspellcpppy::SymSpell::WordSegmentation),
                 "insert spaces in between words in a sentence",
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