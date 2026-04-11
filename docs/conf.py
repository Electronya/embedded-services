import os
import sys

# -- Project -----------------------------------------------------------------

project = "Electronya Embedded Services"
copyright = "Electronya"
author = "Electronya"

# -- Extensions --------------------------------------------------------------

extensions = [
    "breathe",
    "myst_parser",
    "sphinxcontrib.mermaid",
]

suppress_warnings = [
    "myst.xref_missing",  # README.md links to src/ files (fine on GitHub, not in Sphinx)
]

# -- Breathe -----------------------------------------------------------------

breathe_projects = {
    "embedded-services": "../doxygen-out/xml",
}
breathe_default_project = "embedded-services"
breathe_default_members = ("members",)

# -- MyST-Parser -------------------------------------------------------------

myst_enable_extensions = [
    "colon_fence",
]
myst_fence_as_directive = {"mermaid"}
source_suffix = {
    ".rst": "restructuredtext",
    ".md": "markdown",
}

# -- HTML output -------------------------------------------------------------

html_theme = "sphinx_rtd_theme"
html_title = "Electronya Embedded Services"
html_logo = "_static/logo.png"
html_static_path = ["_static"]
html_css_files = ["electronya.css"]

pygments_style = "monokai"

html_theme_options = {
    "navigation_depth": 4,
    "collapse_navigation": False,
    "sticky_navigation": True,
    "includehidden": True,
    "titles_only": False,
}
