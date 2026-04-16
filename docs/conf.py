import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '_theme', 'base'))
from conf_base import *

# -- Project -----------------------------------------------------------------

project   = "Embedded Services"
copyright = "Electronya"
author    = "Electronya"
html_title = "Embedded Services"

# -- Breathe -----------------------------------------------------------------

breathe_projects = {
    "embedded-services": "../doxygen-out/xml",
}
breathe_default_project = "embedded-services"
breathe_default_members = ("members",)
