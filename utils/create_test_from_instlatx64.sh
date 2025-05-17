#!/bin/bash

set -euo pipefail

# Environment variables
export LIBCPUID_NO_WARN=1

# Constants
SHORT_OPTS="r:o:h"
LONG_OPTS="raw-file:,output:,cpuid-tool:,create-test:,help"
GIT_ROOT_DIR="$(git rev-parse --show-toplevel)"
REPORT_FILE="$(mktemp --tmpdir libcpuid-report.XXXXXX)"
MAX_UNCOMPRESSED_SIZE_KB=128

# Variables
raw_file=""
raw_file_tmp_download=""
raw_file_tmp_fix=""
output_dir=""
output_file="/dev/stdout"
cpuid_tool="$GIT_ROOT_DIR/build/cpuid_tool/cpuid_tool"
create_test="$GIT_ROOT_DIR/tests/create_test.py"

# Cleanup on exit
on_exit() {
	rm "$REPORT_FILE"
	[[ -f "$raw_file_tmp_download" ]] && rm "$raw_file_tmp_download"
	[[ -f "$raw_file_tmp_fix"      ]] && rm "$raw_file_tmp_fix"
}

# Display usage
print_help() {
	echo -e "$(basename "$0") [OPTIONS] -r RAW_FILE [-o OUTPUT]\n"
	echo -e "This script helps to download raw file from InstLatx64 and create tests with standardized names.\n"
	echo -e "OPTIONS:"
	echo -e "  -r, --raw-file\tPath to a local raw file OR an URL to a raw file"
	echo -e "  -o, --output\t\tOutput test directory name (for semi-automatic file name) OR output test file name"
	echo -e "      --cpuid-tool\tPath to the cpuid_tool binary (default: $cpuid_tool)"
	echo -e "      --create-test\tPath to the create_test.py script (default: $create_test)"
	echo -e "  -h, --help\t\tThis help"
}

trap on_exit EXIT

# Parse options
eval set -- "$(getopt --name="$0" --options="$SHORT_OPTS" --longoptions="$LONG_OPTS" -- "$@")"
while true; do
	case "$1" in
		-r|--raw-file) raw_file="$2"; shift 2;;
		-o|--output)   [[ -d "$2" ]] && output_dir="$2" || output_file="$2"; shift 2;;
		--cpuid-tool)  cpuid_tool="$2"; shift 2;;
		--create-test) create_test="$2"; shift 2;;
		-h|--help) print_help; exit 0;;
		--) shift; break;;
		*) error "'$1' is not handled."; exit 1;;
	esac
done

if [[ -z "$raw_file" ]]; then
	echo -e "\033[31m$(basename "$0"): --raw-file is mandatory.\033[0m"
	exit 1
fi

# Download a raw file from an URL
if [[ "$raw_file" =~ ^https?:\/\/ ]]; then
	raw_file_tmp_download="$(mktemp --tmpdir libcpuid-raw.XXXXXX)"
	echo -e "\033[34mDownloading '$raw_file' raw file as '$raw_file_tmp_download'...\033[0m"
	wget --quiet "$raw_file" --output-document="$raw_file_tmp_download"
	raw_file="$raw_file_tmp_download"
fi

# Fix raw file if there is no header for logical cores
if [[ -z "$(grep -Ev "CPUID [0-9A-F]{8}" "$raw_file" | xargs)" ]]; then
	raw_file_tmp_fix="$(mktemp --tmpdir libcpuid-raw.XXXXXX)"
	echo -e "\033[34mFixing '$raw_file' raw file as '$raw_file_tmp_fix'...\033[0m"
	logical_cpu=0
	echo "------[ Logical CPU #$logical_cpu ]------" > "$raw_file_tmp_fix"
	while IFS= read -r line || [[ -n "$line" ]]; do
		if [[ -z "$line" ]]; then
			logical_cpu=$((logical_cpu + 1))
			echo -e "\n------[ Logical CPU #$logical_cpu ]------" >> "$raw_file_tmp_fix"
		else
			echo "$line" >> "$raw_file_tmp_fix"
		fi
	done < "$raw_file"
	raw_file="$raw_file_tmp_fix"
fi

echo -e "\033[34mLoading '$raw_file' raw file...\033[0m"
"$cpuid_tool" --load="$raw_file" --report

echo -e "\033[34mCreating '$REPORT_FILE' report file...\033[0m"
"$cpuid_tool" --load="$raw_file" --outfile="$REPORT_FILE" --report

# Generate test file name
if [[ -n "$output_dir" ]]; then
	test_file="$("$cpuid_tool" --load="$raw_file" --brandstr | head -n1 | sed -e 's|([^)]*)||g' -e 's|,||g' -e 's|w/|with|g' | cut -d'@' -f1 | xargs | sed -r 's|\s+|-|g' | tr '[:upper:]' '[:lower:]')"
	if [[ -n "$test_file" ]]; then
		read -r -p "'$test_file' will be the test file name, you can enter a different one if needed (without extension): " test_file_prompt
		test_file="${test_file_prompt:-$test_file}"
	else
		read -r -p "There is no brand string available, please enter file name manually (without extension): " test_file
	fi
	if [[ -z "$test_file" ]]; then
		echo -e "\033[31m$(basename "$0"): no test file name provided.\033[0m"
		exit 1
	fi
	output_file="${output_dir}/${test_file}.test"
	echo -e "\033[34mCreating '$output_file' test file...\033[0m"
fi
"$create_test" "$raw_file" "$REPORT_FILE" > "$output_file"

# Compress big test
if [[ -f "$output_file" ]] && [[ $(stat -c %s "$output_file") -gt $(( 1024 * MAX_UNCOMPRESSED_SIZE_KB )) ]]; then
	echo -e "\033[34mCompressing '$output_file' test file...\033[0m"
	xz --force --compress "$output_file"
fi
