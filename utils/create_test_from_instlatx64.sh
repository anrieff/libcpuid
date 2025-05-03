#!/bin/bash

set -euo pipefail

export LIBCPUID_NO_WARN=1
GIT_ROOT_DIR="$(git rev-parse --show-toplevel)"
CPUID_TOOL="${CPUID_TOOL:-"$GIT_ROOT_DIR/build/cpuid_tool/cpuid_tool"}"
CREATE_TEST="${CREATE_TEST:-"$GIT_ROOT_DIR/tests/create_test.py"}"
RAW_FILE="${1:-"/tmp/raw.txt"}"
OUTPUT_DIR="${2:-}"
REPORT_FILE="$(mktemp --tmpdir libcpuid-report.XXXXXX)"

is_tmp_raw=false
# Download remote file if string contains URL
if [[ "$RAW_FILE" =~ ^https?:\/\/ ]]; then
	is_tmp_raw=true
	raw_file_tmp="$(mktemp --tmpdir libcpuid-raw.XXXXXX)"
	echo -e "\033[1mDownloading '$RAW_FILE' raw file as '$raw_file_tmp'...\033[0m"
	wget --quiet "$RAW_FILE" --output-document="$raw_file_tmp"
	RAW_FILE="$raw_file_tmp"
fi

# Fix raw file if there is no header for logical cores
if [[ -z "$(grep -Ev "CPUID [0-9]{8}" "$RAW_FILE" | xargs)" ]]; then
	is_tmp_raw=true
	raw_file_tmp="$(mktemp --tmpdir libcpuid-raw.XXXXXX)"
	logical_cpu=0
	echo "------[ Logical CPU #$logical_cpu ]------" > "$raw_file_tmp"
	while IFS= read -r line || [[ -n "$line" ]]; do
		if [[ -z "$line" ]]; then
			logical_cpu=$((logical_cpu + 1))
			echo -e "\n------[ Logical CPU #$logical_cpu ]------" >> "$raw_file_tmp"
		else
			echo "$line" >> "$raw_file_tmp"
		fi
	done < "$RAW_FILE"
	RAW_FILE="$raw_file_tmp"
fi

echo -e "\033[1mLoading '$RAW_FILE' raw file...\033[0m"
"$CPUID_TOOL" --load="$RAW_FILE" --report

echo -e "\033[1mCreating '$REPORT_FILE' report file...\033[0m"
"$CPUID_TOOL" --load="$RAW_FILE" --outfile="$REPORT_FILE" --report

if [[ -d "$OUTPUT_DIR" ]]; then
	test_file="$("$CPUID_TOOL" --load="$RAW_FILE" --brandstr | head -n1 | sed -e 's/([^)]*)//g' -e 's/,//g' | cut -d'@' -f1 | xargs | sed -r 's/\s+/-/g' | tr '[:upper:]' '[:lower:]')"
	if [[ -z "$test_file" ]]; then
		read -r -p "There is no brand string available, please enter filename manually: " test_file
	fi
	output_file="${OUTPUT_DIR}/${test_file}.test"
	echo -e "\033[1mCreating '$output_file' test file...\033[0m"
else
	output_file="/dev/stdout"
fi
"$CREATE_TEST" "$RAW_FILE" "$REPORT_FILE" > "$output_file"
rm "$REPORT_FILE"
$is_tmp_raw && rm "$raw_file_tmp"

if [[ -f "$output_file" ]] && [[ $(stat -c %s "$output_file") -gt $(( 1024*128 )) ]]; then
	echo -e "\033[1mCompressing '$output_file' test file...\033[0m"
	xz --force --compress "$output_file"
fi
