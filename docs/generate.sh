#!/bin/bash
# Generate documentation for embedded-services.
# Run from any directory — the script always works relative to its own location.
#
# Stack: Doxygen (XML) → Breathe → Sphinx (HTML)
#
# System dependencies (install once):
#   sudo pacman -S doxygen          # Arch Linux
#   sudo apt install doxygen        # Debian/Ubuntu
#
# Python dependencies are managed in a local .venv:
#   python3 -m venv .venv
#   source .venv/bin/activate
#   pip install sphinx breathe myst-parser sphinx-rtd-theme sphinxcontrib-mermaid

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Activate venv if present and sphinx-build is not already on PATH
if ! command -v sphinx-build &>/dev/null; then
    if [ -f "$SCRIPT_DIR/../.venv/bin/activate" ]; then
        # shellcheck disable=SC1091
        source "$SCRIPT_DIR/../.venv/bin/activate"
    else
        echo "Error: sphinx-build not found. Create a venv or activate one first." >&2
        echo "  python3 -m venv ../.venv && source ../.venv/bin/activate" >&2
        echo "  pip install sphinx breathe myst-parser sphinx-rtd-theme sphinxcontrib-mermaid" >&2
        exit 1
    fi
fi

if ! command -v doxygen &>/dev/null; then
    echo "Error: doxygen not found. Install it:" >&2
    echo "  sudo pacman -S doxygen   # Arch Linux" >&2
    echo "  sudo apt install doxygen # Debian/Ubuntu" >&2
    exit 1
fi

echo "Step 1/2 — Running Doxygen (XML)..."
doxygen Doxyfile

echo "Step 2/2 — Running Sphinx..."
sphinx-build -b html . ../sphinx-out

echo ""
echo "Done. Open sphinx-out/index.html to view the documentation."
