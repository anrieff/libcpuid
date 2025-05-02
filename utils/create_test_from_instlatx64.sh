#!/bin/bash

set -euo pipefail

GIT_ROOT_DIR="$(git rev-parse --show-toplevel)"
CPUID_TOOL="${CPUID_TOOL:-"$GIT_ROOT_DIR/build/cpuid_tool/cpuid_tool"}"
CREATE_TEST="${CREATE_TEST:-"$GIT_ROOT_DIR/tests/create_test.py"}"
RAW_FILE="${1:-"/tmp/raw.txt"}"
OUTPUT_DIR="${2:-}"
REPORT_FILE="$(mktemp --tmpdir libcpuid-report.XXXXXX)"

echo -e "\033[1mLoading '$RAW_FILE' raw file...\033[0m"
"$CPUID_TOOL" --load="$RAW_FILE" --report

echo -e "\033[1mCreating '$REPORT_FILE' report file...\033[0m"
"$CPUID_TOOL" --load="$RAW_FILE" --outfile="$REPORT_FILE" --report

if [[ -d "$OUTPUT_DIR" ]]; then
	test_file="$("$CPUID_TOOL" --load="$RAW_FILE" --brandstr | head -n1 | sed -e 's/([^)]*)//g' -e 's/,//g' | cut -d'@' -f1 | xargs | sed -r 's/\s+/-/g' | tr '[:upper:]' '[:lower:]')"
	output_file="${OUTPUT_DIR}/${test_file}.test"
	echo -e "\033[1mCreating '$output_file' test file...\033[0m"
else
	output_file="/dev/stdout"
fi
"$CREATE_TEST" "$RAW_FILE" "$REPORT_FILE" > "$output_file"
rm "$REPORT_FILE"

if [[ -f "$output_file" ]] && [[ $(stat -c %s "$output_file") -gt $(( 1024*128 )) ]]; then
	xz --force --compress "$output_file"
fi
