#!/bin/bash
# Generate documentation for embedded-services.
# Run from any directory — the script always works relative to its own location.
#
# Stack: Doxygen (XML) → Breathe → Sphinx (HTML)
#
# Dependencies (install once):
#   pip install sphinx breathe myst-parser furo

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Step 1/2 — Running Doxygen (XML)..."
doxygen Doxyfile

echo "Step 2/2 — Running Sphinx..."
sphinx-build -b html . ../sphinx-out

echo ""
echo "Done. Open sphinx-out/index.html to view the documentation."
