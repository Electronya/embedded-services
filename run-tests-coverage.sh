#!/bin/bash
#
# Run tests and generate filtered coverage report
#
# This script runs the test suite with coverage and generates a filtered
# coverage report showing only project source files (src/), excluding
# LOG macro branches which always show 50% due to compile-time conditionals.
#
# Usage:
#   ./scripts/run-tests-coverage.sh [test-path]
#
# Examples:
#   ./scripts/run-tests-coverage.sh                    # Run all tests
#   ./scripts/run-tests-coverage.sh tests/datastore    # Run specific test directory
#

set -e

# Change to the embedded-services directory (script is at repo root)
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_DIR"

# Get test path from argument or default to all tests
TEST_PATH="${1:-tests}"

echo "=== Running Tests with Coverage ==="
echo "Test path: $TEST_PATH"
echo ""

# Clean previous test results
rm -rf twister-out twister-out.*

# Run tests with coverage
west twister -T "$TEST_PATH" --coverage --gcov-tool /usr/bin/gcov -v

echo ""
echo "=== Generating Filtered Coverage Report ==="

# Create directory for filtered report
mkdir -p twister-out/filteredReport

# Check if gcovr is installed
if ! command -v gcovr &> /dev/null; then
    echo "Error: gcovr is not installed"
    echo "Install with: pip install gcovr"
    exit 1
fi

# Generate filtered HTML report with only project source files
# Exclude LOG macro branches from coverage (they always show 50% due to compile-time conditionals)
gcovr --filter 'src/' \
      --exclude-branches-by-pattern '.*LOG_.*' \
      twister-out \
      --html-details \
      -o twister-out/filteredReport/index.html

echo ""
echo "=== Coverage Summary (Project Source Files Only) ==="

# Display coverage summary
gcovr --filter 'src/' \
      --exclude-branches-by-pattern '.*LOG_.*' \
      twister-out \
      -s

echo ""
echo "=== Test Results Summary ==="

# Extract test results
if [ -f twister-out/twister.xml ]; then
    TOTAL_TESTS=$(grep -o '<testcase' twister-out/twister.xml 2>/dev/null | wc -l)
    FAILED_TESTS=$(grep -o '<failure' twister-out/twister.xml 2>/dev/null | wc -l)
    TOTAL_TESTS=${TOTAL_TESTS:-0}
    FAILED_TESTS=${FAILED_TESTS:-0}
    PASSED_TESTS=$((TOTAL_TESTS - FAILED_TESTS))

    echo "Total tests:  $TOTAL_TESTS"
    echo "Passed:       $PASSED_TESTS"
    echo "Failed:       $FAILED_TESTS"

    if [ "$FAILED_TESTS" -eq 0 ]; then
        echo "Status:       ✓ PASSING"
    else
        echo "Status:       ✗ FAILING"
    fi
else
    echo "No test results found"
fi

echo ""
echo "=== Reports Generated ==="
echo "HTML coverage report: twister-out/filteredReport/index.html"
echo "Test results (JSON):  twister-out/twister.json"
echo "Test results (XML):   twister-out/twister.xml"
echo ""

# Open HTML report in browser if available (optional)
if command -v xdg-open &> /dev/null; then
    read -p "Open coverage report in browser? [y/N] " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        xdg-open "twister-out/filteredReport/index.html"
    fi
fi
