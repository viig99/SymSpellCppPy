#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/filesystem.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <fstream>
#include <sstream>

#include "library.h"

namespace nb = nanobind;
namespace unicode = symspellcpppy::unicode;
using symspellcpppy::Info;
using symspellcpppy::SymSpell;
using symspellcpppy::Verbosity;

namespace nanobind::detail
{
    template <> struct type_caster<xstring>
    {
        NB_TYPE_CASTER(xstring, const_name("str"))

        bool from_python(handle source, uint8_t flags, cleanup_list *cleanup) noexcept
        {
            type_caster<std::string> utf8;
            if (!utf8.from_python(source, flags, cleanup))
                return false;
            try
            {
                value = unicode::decode(utf8.value);
                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        static handle from_cpp(const xstring &value, rv_policy policy, cleanup_list *cleanup)
        {
            return type_caster<std::string>::from_cpp(unicode::encode(value), policy, cleanup);
        }
    };
}

namespace
{
    xchar Separator(const xstring &text)
    {
        if (text.size() != 1)
            throw std::invalid_argument("separator must be a single character");
        return text.front();
    }

    std::string FormatSuggestion(const SuggestItem &item)
    {
        return unicode::encode(item.term) + ", " + std::to_string(item.distance) + ", " +
               std::to_string(item.count);
    }

    class Buffer
    {
    public:
        explicit Buffer(nb::handle source)
        {
            if (PyObject_GetBuffer(source.ptr(), &view, PyBUF_STRIDES) != 0)
                throw nb::python_error();
        }

        ~Buffer() { PyBuffer_Release(&view); }

        Buffer(const Buffer &) = delete;
        Buffer &operator=(const Buffer &) = delete;

        Py_buffer view{};
    };

    std::istringstream ReadTextStream(nb::handle corpus)
    {
        nb::object data = corpus.attr("read")();
        if (nb::isinstance<nb::bytes>(data))
        {
            const auto bytes = nb::cast<nb::bytes>(data);
            return std::istringstream(std::string(bytes.c_str(), bytes.size()));
        }
        return std::istringstream(nb::cast<std::string>(data));
    }
}

NB_MODULE(_core, m)
{
    m.doc() = "Spelling correction and word segmentation using the Symmetric Delete algorithm.";

    nb::class_<Info>(m, "Info", "Result of word segmentation.")
        .def(nb::init<>())
        .def("set", &Info::set, "Replace the fields of this result.", nb::arg("segmented_string"),
             nb::arg("corrected_string"), nb::arg("distance_sum"), nb::arg("log_prob_sum"))
        .def("get_segmented", &Info::getSegmented)
        .def("get_corrected", &Info::getCorrected)
        .def("get_distance", &Info::getDistance)
        .def("get_probability", &Info::getProbability)
        .def_prop_ro("segmented_string", &Info::getSegmented, "Input with word boundaries inserted.")
        .def_prop_ro("corrected_string", &Info::getCorrected, "Segmented and corrected text.")
        .def_prop_ro("distance_sum", &Info::getDistance, "Total edit cost of the segmentation.")
        .def_prop_ro("log_prob_sum", &Info::getProbability, "Sum of base-10 log probabilities.")
        .def("__repr__", [](const Info &result) {
            return "<Info corrected_string ='" + unicode::encode(result.getCorrected()) + "'>";
        });

    nb::class_<SuggestItem>(m, "SuggestItem", "A spelling suggestion, its edit distance and frequency.")
        .def(nb::init<xstring, int, int64_t>(), nb::arg("term"), nb::arg("distance"), nb::arg("count"))
        .def("__eq__", &SuggestItem::Equals, nb::is_operator())
        .def("__lt__", &SuggestItem::compare, nb::is_operator())
        .def("__repr__", &FormatSuggestion)
        .def("__str__", &FormatSuggestion)
        .def_rw("term", &SuggestItem::term)
        .def_rw("distance", &SuggestItem::distance)
        .def_rw("count", &SuggestItem::count);

    nb::enum_<Verbosity>(m, "Verbosity", "Controls how many spelling suggestions are returned.", nb::is_arithmetic())
        .value("TOP", Verbosity::Top, "The highest-frequency suggestion at the smallest edit distance.")
        .value("CLOSEST", Verbosity::Closest, "All suggestions at the smallest edit distance.")
        .value("ALL", Verbosity::All, "All suggestions within the allowed edit distance.")
        .export_values();

    nb::class_<SymSpell>(m, "SymSpell", "A mutable dictionary for spelling correction.")
        .def(nb::init<int, int, int, int, unsigned char>(),
             nb::arg("max_dictionary_edit_distance") = DEFAULT_MAX_EDIT_DISTANCE,
             nb::arg("prefix_length") = DEFAULT_PREFIX_LENGTH,
             nb::arg("count_threshold") = DEFAULT_COUNT_THRESHOLD,
             nb::arg("initial_capacity") = DEFAULT_INITIAL_CAPACITY,
             nb::arg("compact_level") = DEFAULT_COMPACT_LEVEL)
        .def("word_count", &SymSpell::WordCount, "Return the number of indexed words.")
        .def("max_length", &SymSpell::MaxLength, "Return the longest indexed word length in code points.")
        .def("entry_count", &SymSpell::EntryCount, "Return the number of delete hash buckets.")
        .def("count_threshold", &SymSpell::CountThreshold, "Return the minimum count for indexed words.")
        .def("max_dictionary_edit_distance", &SymSpell::MaxDictionaryEditDistance, "Return the index distance limit.")
        .def("prefix_length", &SymSpell::PrefixLength, "Return the prefix length used by the index.")
        .def("create_dictionary_entry", [](SymSpell &spell, const xstring &key, int64_t count) {
            return spell.CreateDictionaryEntry(Helpers::string_lower(key), count);
        }, "Add to a word's count and return whether the word was newly indexed.", nb::arg("key"), nb::arg("count"))
        .def("delete_dictionary_entry", [](SymSpell &spell, const xstring &key) {
            if (spell.DeleteDictionaryEntry(key))
                return true;
            const auto lowercase = Helpers::string_lower(key);
            return lowercase != key && spell.DeleteDictionaryEntry(lowercase);
        }, "Remove an indexed term and return whether it existed.", nb::arg("key"))
        .def("load_dictionary", [](SymSpell &spell, const std::filesystem::path &corpus, int term, int count,
                                   const xstring &separator) {
            return spell.LoadDictionary(corpus, term, count, Separator(separator));
        }, "Merge a UTF-8 file containing words and counts.", nb::arg("corpus"), nb::arg("term_index"),
             nb::arg("count_index"), nb::arg("separator") = " ")
        .def("load_dictionary_stream", [](SymSpell &spell, nb::handle corpus, int term, int count,
                                          const xstring &separator) {
            auto stream = ReadTextStream(corpus);
            return spell.LoadDictionary(stream, term, count, Separator(separator));
        }, "Merge words and counts from a readable text or UTF-8 binary stream.", nb::arg("corpus"),
             nb::arg("term_index"), nb::arg("count_index"), nb::arg("separator") = " ")
        .def("load_bigram_dictionary", [](SymSpell &spell, const std::filesystem::path &corpus, int term,
                                          int count, const xstring &separator) {
            return spell.LoadBigramDictionary(corpus, term, count, Separator(separator));
        }, "Merge a UTF-8 file containing word pairs and counts.", nb::arg("corpus"), nb::arg("term_index"),
             nb::arg("count_index"), nb::arg("separator") = " ")
        .def("load_bigram_dictionary_stream", [](SymSpell &spell, nb::handle corpus, int term, int count,
                                                 const xstring &separator) {
            auto stream = ReadTextStream(corpus);
            return spell.LoadBigramDictionary(stream, term, count, Separator(separator));
        }, "Merge word pairs and counts from a readable text or UTF-8 binary stream.", nb::arg("corpus"),
             nb::arg("term_index"), nb::arg("count_index"), nb::arg("separator") = " ")
        .def("create_dictionary", nb::overload_cast<const std::filesystem::path &>(&SymSpell::CreateDictionary),
             "Build word frequencies from a plain UTF-8 text file.", nb::arg("corpus"))
        .def("create_dictionary_stream", [](SymSpell &spell, nb::handle corpus) {
            auto stream = ReadTextStream(corpus);
            return spell.CreateDictionary(stream);
        }, "Build word frequencies from a readable text or UTF-8 binary stream.", nb::arg("corpus"))
        .def("purge_below_threshold_words", &SymSpell::PurgeBelowThresholdWords,
             "Discard accumulated counts for words that have not reached the indexing threshold.")
        .def("lookup", [](SymSpell &spell, const xstring &input, Verbosity verbosity,
                          std::optional<int> distance, bool includeUnknown, bool transferCasing) {
            return spell.Lookup(input, verbosity, distance.value_or(spell.MaxDictionaryEditDistance()),
                                includeUnknown, transferCasing);
        }, "Find spellings for one word, ranked by distance and frequency.", nb::arg("input"), nb::arg("verbosity"),
             nb::arg("max_edit_distance") = nb::none(), nb::arg("include_unknown") = false,
             nb::arg("transfer_casing") = false)
        .def("lookup_compound", [](SymSpell &spell, const xstring &input, std::optional<int> distance, bool casing) {
            return spell.LookupCompound(input, distance.value_or(spell.MaxDictionaryEditDistance()), casing);
        }, "Correct multiple words, including misplaced or missing spaces.", nb::arg("input"),
             nb::arg("max_edit_distance") = nb::none(), nb::arg("transfer_casing") = false)
        .def("word_segmentation", [](SymSpell &spell, const xstring &input, std::optional<int> distance,
                                     std::optional<int> wordLength) {
            const auto maxDistance = distance.value_or(spell.MaxDictionaryEditDistance());
            return wordLength ? spell.WordSegmentation(input, maxDistance, *wordLength)
                              : spell.WordSegmentation(input, maxDistance);
        }, "Normalize, segment and correct text.", nb::arg("input"), nb::arg("max_edit_distance") = nb::none(),
             nb::arg("max_segmentation_word_length") = nb::none())
        .def("save_pickle", [](const SymSpell &spell, const std::filesystem::path &path) {
            std::ofstream stream(path, std::ios::binary | std::ios::trunc);
            if (!stream)
                throw std::invalid_argument("Cannot open archive for writing");
            spell.Save(stream);
            stream.close();
            if (!stream)
                throw std::runtime_error("Could not write archive");
        }, "Overwrite a file with a binary dictionary snapshot.", nb::arg("filepath"))
        .def("load_pickle", [](SymSpell &spell, const std::filesystem::path &path) {
            std::ifstream stream(path, std::ios::binary);
            if (!stream)
                throw std::invalid_argument("Cannot open archive for reading");
            spell.Load(stream);
        }, "Restore a dictionary snapshot from a trusted file.", nb::arg("filepath"))
        .def("save_pickle_bytes", [](const SymSpell &spell) {
            std::ostringstream stream(std::ios::binary);
            spell.Save(stream);
            const auto data = std::move(stream).str();
            return nb::bytes(data.data(), data.size());
        }, "Return a binary dictionary snapshot as bytes.")
        .def("load_pickle_bytes", [](SymSpell &spell, nb::handle source) {
            Buffer buffer(source);
            const auto &view = buffer.view;
            if (view.ndim != 1 || !PyBuffer_IsContiguous(&view, 'C'))
                throw std::invalid_argument("Expected a one-dimensional contiguous buffer");
            std::string data;
            if (view.len > 0)
                data.assign(static_cast<const char *>(view.buf), view.len);
            std::istringstream stream(std::move(data), std::ios::binary);
            spell.Load(stream);
        }, "Restore a dictionary snapshot from a trusted contiguous buffer.", nb::arg("bytes"));
}
