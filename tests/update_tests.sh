#!/bin/bash

TESTS_DIR="$(dirname "$(realpath "$0")")"
LIBCPUID_DIR="$(dirname "$TESTS_DIR")"
LIBCPUID_CPUID_TOOL="$(find "$LIBCPUID_DIR" -name cpuid_tool -type f)"
LIBCPUID_CREATE_TEST="$(find "$LIBCPUID_DIR" -name create_test.py -type f)"

test_files=$(find "$TESTS_DIR" -name '*.test')
for test_file in $test_files; do
	echo "Update ${test_file#"${TESTS_DIR}/"}..."
	TMP_DIR="$(mktemp --directory --tmpdir libcpuid.XXXXXX)"
	while IFS= read -r line; do
		if [[ "$line" = "--------------------------------------------------------------------------------" ]]; then
			break
		fi
		echo "$line" >> "$TMP_DIR/raw.txt"
	done < "$test_file"
	"$LIBCPUID_CPUID_TOOL" --load="$TMP_DIR/raw.txt" --report > "$TMP_DIR/report.txt"
	"$LIBCPUID_CREATE_TEST" "$TMP_DIR/raw.txt" "$TMP_DIR/report.txt" > "$test_file"
	rm -rf "$TMP_DIR"
done
