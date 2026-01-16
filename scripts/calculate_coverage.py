#!/usr/bin/env python3
"""
Calculate code coverage for specific source files.

This script can extract coverage in two modes:
1. From gcovr summary JSON (--json mode)
2. By running gcov directly on .gcda files (--gcov-dir mode)

The gcov mode is useful when source files are #include'd directly into test
files, as gcovr may not properly resolve the original source file paths.

Usage:
    calculate_coverage.py --json <coverage_json> [-f PATTERN]...
    calculate_coverage.py --gcov-dir <build_dir> [-f PATTERN]...

Options:
    --json          Path to gcovr coverage_summary.json file
    --gcov-dir      Path to build directory containing .gcda files
    --filter, -f    Path pattern to include (can be specified multiple times)
                    Files must contain this pattern to be included.
                    Default: 'src/' if no filters specified

Examples:
    # From gcovr JSON
    calculate_coverage.py --json coverage_summary.json -f 'src/'

    # From gcov data files
    calculate_coverage.py --gcov-dir twister-out -f 'src/adcAcquisition'

Output:
    Prints coverage percentage (e.g., "85.3") to stdout.
    Exit code 0 on success, 1 on error.
"""

import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path


def calculate_from_json(coverage_file: str, filters: list[str]) -> float | None:
    """
    Calculate line coverage from gcovr JSON for files matching filters.
    """
    try:
        with open(coverage_file, 'r') as f:
            data = json.load(f)
    except (FileNotFoundError, json.JSONDecodeError) as e:
        print(f"Error reading coverage file: {e}", file=sys.stderr)
        return None

    total_lines = 0
    covered_lines = 0
    matched_files = []

    for file_entry in data.get('files', []):
        filename = file_entry.get('filename', '')

        if any(pattern in filename for pattern in filters):
            line_total = file_entry.get('line_total', 0)
            line_covered = file_entry.get('line_covered', 0)

            if line_total > 0:
                total_lines += line_total
                covered_lines += line_covered
                matched_files.append(filename)

    if total_lines == 0:
        print(f"Warning: No matching files found for filters: {filters}", file=sys.stderr)
        return None

    coverage_pct = (covered_lines / total_lines) * 100
    print(f"Matched {len(matched_files)} files, {covered_lines}/{total_lines} lines covered",
          file=sys.stderr)

    return coverage_pct


def calculate_from_gcov(build_dir: str, filters: list[str]) -> float | None:
    """
    Calculate line coverage by running gcov on .gcda files.

    This method properly handles source files that are #include'd into test files.
    """
    build_path = Path(build_dir)

    # Find all .gcda files
    gcda_files = list(build_path.rglob("*.gcda"))

    if not gcda_files:
        print(f"Warning: No .gcda files found in {build_dir}", file=sys.stderr)
        return None

    # Regex to parse gcov output: "Lines executed:XX.XX% of YYY"
    coverage_pattern = re.compile(r"Lines executed:(\d+\.\d+)% of (\d+)")
    file_pattern = re.compile(r"File '([^']+)'")

    file_coverage = {}  # {filename: (covered_lines, total_lines)}

    for gcda_file in gcda_files:
        gcda_dir = gcda_file.parent

        try:
            result = subprocess.run(
                ["gcov", gcda_file.name],
                cwd=gcda_dir,
                capture_output=True,
                text=True,
                timeout=30
            )
            output = result.stdout + result.stderr
        except (subprocess.TimeoutExpired, FileNotFoundError) as e:
            print(f"Warning: Failed to run gcov on {gcda_file}: {e}", file=sys.stderr)
            continue

        # Parse gcov output
        current_file = None
        for line in output.split('\n'):
            file_match = file_pattern.match(line)
            if file_match:
                current_file = file_match.group(1)
                continue

            cov_match = coverage_pattern.search(line)
            if cov_match and current_file:
                percent = float(cov_match.group(1))
                total = int(cov_match.group(2))
                covered = int(total * percent / 100)

                # Store or update coverage for this file
                if current_file in file_coverage:
                    # Aggregate coverage (take max in case of multiple test runs)
                    prev_cov, prev_total = file_coverage[current_file]
                    if total > prev_total:
                        file_coverage[current_file] = (covered, total)
                else:
                    file_coverage[current_file] = (covered, total)

                current_file = None

    # Filter and aggregate results
    total_lines = 0
    covered_lines = 0
    matched_files = []

    for filename, (cov, total) in file_coverage.items():
        if any(pattern in filename for pattern in filters):
            total_lines += total
            covered_lines += cov
            matched_files.append(f"{filename}: {cov}/{total}")

    if total_lines == 0:
        print(f"Warning: No matching files found for filters: {filters}", file=sys.stderr)
        return None

    coverage_pct = (covered_lines / total_lines) * 100

    print(f"Matched {len(matched_files)} files, {covered_lines}/{total_lines} lines covered",
          file=sys.stderr)
    for f in matched_files:
        print(f"  {f}", file=sys.stderr)

    return coverage_pct


def main():
    parser = argparse.ArgumentParser(
        description='Calculate code coverage for filtered source files.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )

    source_group = parser.add_mutually_exclusive_group(required=True)
    source_group.add_argument(
        '--json',
        metavar='FILE',
        help='Path to gcovr coverage_summary.json file'
    )
    source_group.add_argument(
        '--gcov-dir',
        metavar='DIR',
        help='Path to build directory containing .gcda files'
    )

    parser.add_argument(
        '-f', '--filter',
        action='append',
        dest='filters',
        metavar='PATTERN',
        help='Path pattern to include (can be repeated, OR logic)'
    )

    args = parser.parse_args()

    # Default filter if none specified
    filters = args.filters if args.filters else ['src/']

    if args.json:
        coverage = calculate_from_json(args.json, filters)
    else:
        coverage = calculate_from_gcov(args.gcov_dir, filters)

    if coverage is not None:
        print(f"{coverage:.1f}")
        return 0
    else:
        return 1


if __name__ == '__main__':
    sys.exit(main())
