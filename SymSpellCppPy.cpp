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
        ---------------------------
        .. currentmodule:: SymSpellCppPy
        .. autosummary::
           :toctree: _generate
           symspell
    )pbdoc";

    py::class_<symspellcpppy::Info>(m, "Info")
            .def(py::init<>())
            .def("set", &symspellcpppy::Info::set, "Set Info properties", py::arg("segmented_string"), py::arg("corrected_string"),
                 py::arg("distance_sum"), py::arg("log_prob_sum"))
            .def("get_segmented", &symspellcpppy::Info::getSegmented)
            .def("get_corrected", &symspellcpppy::Info::getCorrected)
            .def("get_distance", &symspellcpppy::Info::getDistance)
            .def("get_probability", &symspellcpppy::Info::getProbability)
            .def("__repr__",
                 [](const symspellcpppy::Info &a) {
                     return "<Info corrected string ='" + a.getCorrected() + "'>";
                 }
            );

    py::enum_<symspellcpppy::Verbosity>(m, "Verbosity")
            .value("Top", symspellcpppy::Verbosity::Top, "Get the top-k matches")
            .value("Closest", symspellcpppy::Verbosity::Closest, "Get the closest match")
            .value("All", symspellcpppy::Verbosity::All, "Get all matches ranked on basis of edit distance.")
            .export_values();

    py::class_<symspellcpppy::SymSpell>(m, "SymSpell")
            .def(py::init<int, int, int, int, unsigned char>(), "SymSpell builder options",
                 py::arg("initial_capacity") = DEFAULT_INITIAL_CAPACITY,
                 py::arg("max_dictionary_edit_distance") = DEFAULT_MAX_EDIT_DISTANCE,
                 py::arg("prefix_length") = DEFAULT_PREFIX_LENGTH,
                 py::arg("count_threshold") = DEFAULT_COUNT_THRESHOLD,
                 py::arg("compact_level") = DEFAULT_COMPACT_LEVEL
            )
            .def("load_bigram_dictionary", py::overload_cast<const std::string &, int, int, xchar>(
                    &symspellcpppy::SymSpell::LoadBigramDictionary),
                 "Load the bi-gram dictionary",
                 py::arg("corpus"),
                 py::arg("term_index"),
                 py::arg("count_index"),
                 py::arg("sep") = DEFAULT_SEPARATOR_CHAR)
            .def("load_dictionary", py::overload_cast<const std::string &, int, int, xchar>(
                    &symspellcpppy::SymSpell::LoadDictionary),
                 "Load the dictionary",
                 py::arg("corpus"),
                 py::arg("term_index"),
                 py::arg("count_index"),
                 py::arg("sep") = DEFAULT_SEPARATOR_CHAR)
            .def("create_dictionary", py::overload_cast<const std::string &>(
                    &symspellcpppy::SymSpell::CreateDictionary),
                 "create dictionary",
                 py::arg("corpus"))
            .def("purge_below_threshold_words", &symspellcpppy::SymSpell::PurgeBelowThresholdWords,
                 "purge below threshold words")
            .def("lookup_term", py::overload_cast<xstring, symspellcpppy::Verbosity>(
                    &symspellcpppy::SymSpell::LookupTerm),
                 "lookup word in dictionary",
                 py::arg("input"),
                 py::arg("verbosity"))
            .def("lookup_term", py::overload_cast<xstring, symspellcpppy::Verbosity, int>(
                    &symspellcpppy::SymSpell::LookupTerm),
                 "lookup word in dictionary",
                 py::arg("input"),
                 py::arg("verbosity"),
                 py::arg("max_edit_distance"))
            .def("lookup_term", py::overload_cast<xstring, symspellcpppy::Verbosity, int, bool>(
                    &symspellcpppy::SymSpell::LookupTerm),
                 "lookup word in dictionary",
                 py::arg("input"),
                 py::arg("verbosity"),
                 py::arg("max_edit_distance"),
                 py::arg("include_unknown"))
            .def("lookup_compound_term", py::overload_cast<const xstring&, int>(
                    &symspellcpppy::SymSpell::LookupCompoundTerm),
                 "lookup compound words from the dictionary",
                 py::arg("input"),
                 py::arg("max_edit_distance"))
            .def("lookup_compound_term", py::overload_cast<const xstring&>(
                    &symspellcpppy::SymSpell::LookupCompoundTerm),
                 "lookup compound words from the dictionary",
                 py::arg("input"))
            .def("word_segmentation", py::overload_cast<const xstring&>(
                    &symspellcpppy::SymSpell::WordSegmentation),
                 "insert spaces in between words in a sentence",
                 py::arg("input"))
            .def("word_segmentation", py::overload_cast<const xstring&, int>(
                    &symspellcpppy::SymSpell::WordSegmentation),
                 "insert spaces in between words in a sentence",
                 py::arg("input"),
                 py::arg("max_edit_distance"))
            .def("word_segmentation", py::overload_cast<const xstring&, int, int>(
                    &symspellcpppy::SymSpell::WordSegmentation),
                 "insert spaces in between words in a sentence",
                 py::arg("input"),
                 py::arg("max_edit_distance"),
                 py::arg("max_seg_word_length"));

}