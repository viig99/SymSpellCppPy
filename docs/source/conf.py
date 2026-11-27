"""Sphinx configuration."""

from importlib.metadata import version as package_version

project = "SymSpellCppPy"
author = "Arjun Variar and Mohit Tare"
release = package_version(project)
extensions = ["sphinx.ext.doctest"]
html_theme = "furo"
html_title = project
html_show_sourcelink = False
html_theme_options = {
    "light_css_variables": {
        "color-brand-primary": "#2546a2",
        "color-brand-content": "#2849a9",
    },
    "dark_css_variables": {
        "color-brand-primary": "#adbdff",
        "color-brand-content": "#adbdff",
    },
    "source_repository": "https://github.com/viig99/SymSpellCppPy/",
    "source_branch": "master",
    "source_directory": "docs/source/",
}
nitpicky = True
