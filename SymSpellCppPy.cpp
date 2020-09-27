//
// Created by vigi99 on 27/09/20.
//

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "library.h"

namespace py = pybind11;

PYBIND11_MODULE(SymSpellCppPy, m) {
    m.doc() = "Pybind11 binding for SymSpellPy";

    py::class_<symspellcpppy::Info>(m, "Info")
            .def(py::init<>())
            .def("set", &symspellcpppy::Info::set, "Set Info properties", py::arg("seg"), py::arg("cor"),
                 py::arg("dist"), py::arg("prob"))
            .def("getSegmented", &symspellcpppy::Info::getSegmented)
            .def("getCorrected", &symspellcpppy::Info::getCorrected)
            .def("getDistance", &symspellcpppy::Info::getDistance)
            .def("getProbability", &symspellcpppy::Info::getProbability)
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
                 py::arg("initialCapacity") = DEFAULT_INITIAL_CAPACITY,
                 py::arg("maxDictionaryEditDistance") = DEFAULT_MAX_EDIT_DISTANCE,
                 py::arg("prefixLength") = DEFAULT_PREFIX_LENGTH,
                 py::arg("countThreshold") = DEFAULT_COUNT_THRESHOLD,
                 py::arg("compactLevel") = DEFAULT_COMPACT_LEVEL
            )
            .def("LoadBigramDictionary", py::overload_cast<const std::string &, int, int, xchar>(
                    &symspellcpppy::SymSpell::LoadBigramDictionary),
                 "Load the bigram dictionary",
                 py::arg("corpus"),
                 py::arg("termIndex"),
                 py::arg("countIndex"),
                 py::arg("separatorChars") = DEFAULT_SEPARATOR_CHAR)
            .def("LoadDictionary", py::overload_cast<const std::string &, int, int, xchar>(
                    &symspellcpppy::SymSpell::LoadDictionary),
                 "Load the dictionary",
                 py::arg("corpus"),
                 py::arg("termIndex"),
                 py::arg("countIndex"),
                 py::arg("separatorChars") = DEFAULT_SEPARATOR_CHAR)
            .def("CreateDictionary", py::overload_cast<const std::string &>(
                    &symspellcpppy::SymSpell::CreateDictionary),
                 "create dictionary",
                 py::arg("corpus"))
            .def("PurgeBelowThresholdWords", &symspellcpppy::SymSpell::PurgeBelowThresholdWords,
                 "purge below threshold words")
            .def("LookupTerm", py::overload_cast<xstring, symspellcpppy::Verbosity>(
                    &symspellcpppy::SymSpell::LookupTerm),
                 "lookup word in dictionary",
                 py::arg("input"),
                 py::arg("verbosity"))
            .def("LookupTerm", py::overload_cast<xstring, symspellcpppy::Verbosity, int>(
                    &symspellcpppy::SymSpell::LookupTerm),
                 "lookup word in dictionary",
                 py::arg("input"),
                 py::arg("verbosity"),
                 py::arg("maxEditDistance"))
            .def("LookupTerm", py::overload_cast<xstring, symspellcpppy::Verbosity, int, bool>(
                    &symspellcpppy::SymSpell::LookupTerm),
                 "lookup word in dictionary",
                 py::arg("input"),
                 py::arg("verbosity"),
                 py::arg("maxEditDistance"),
                 py::arg("includeUnknown"))
            .def("LookupCompoundTerm", py::overload_cast<const xstring&, int>(
                    &symspellcpppy::SymSpell::LookupCompoundTerm),
                 "lookup compound words from the dictionary",
                 py::arg("input"),
                 py::arg("editDistanceMax"))
            .def("LookupCompoundTerm", py::overload_cast<const xstring&>(
                    &symspellcpppy::SymSpell::LookupCompoundTerm),
                 "lookup compound words from the dictionary",
                 py::arg("input"))
            .def("WordSegmentation", py::overload_cast<const xstring&>(
                    &symspellcpppy::SymSpell::WordSegmentation),
                 "insert spaces in between words in a sentence",
                 py::arg("input"))
            .def("WordSegmentation", py::overload_cast<const xstring&, int>(
                    &symspellcpppy::SymSpell::WordSegmentation),
                 "insert spaces in between words in a sentence",
                 py::arg("input"),
                 py::arg("maxEditDistance"))
            .def("WordSegmentation", py::overload_cast<const xstring&, int, int>(
                    &symspellcpppy::SymSpell::WordSegmentation),
                 "insert spaces in between words in a sentence",
                 py::arg("input"),
                 py::arg("maxEditDistance"),
                 py::arg("maxSegmentationWordLength"));

}