#!/bin/bash
# This script extracts some data from Arm Architecture Reference Manual PDF file
# PDF file can be downloaded here: https://developer.arm.com/documentation/ddi0487/latest/

print_help() {
	echo "$(basename "$0") PDF_FILE"
	echo -e "\n\033[1mPDF_FILE:\033[0m Arm Architecture Reference Manual PDF file"
}

if [[ $# -lt 1 ]]; then
	print_help
	exit 1
fi

PDF_FILE="$1"
TMP_DIR="$(mktemp -d -t libcpuid.XXXXXX)"
TMP_LIBCPUID_H="$TMP_DIR/cpu_feature.h"
TMP_CPUID_MAIN_C="$TMP_DIR/matchtable.c"

echo "typedef enum {" > "$TMP_LIBCPUID_H"
echo "	matchtable[] = {" > "$TMP_CPUID_MAIN_C"

IFS='
'
for line in $(pdfgrep --cache -e 'A[0-9]+\.[0-9]+\.[0-9]+\s+The Armv[0-9]\.[0-9] architecture extension' -e '                  FEAT_[a-zA-Z0-9_]+, ' "$PDF_FILE"); do
	if [[ "$line" == "A"* ]]; then
		echo "$line"
		echo -e "\t/* $line */" >> "$TMP_LIBCPUID_H"
		echo -e "\t\t/* $line */" >> "$TMP_CPUID_MAIN_C"
	elif [[ "$line" == "                  FEAT_"* ]]; then
		feat="$(echo "$line" | awk '{ print $1 }' | cut -d, -f1)"
		#pdfgrep --cache "$feat implements the functionality identified by the value" "$PDF_FILE"
		feat_upper="${feat^^}"
		feat_lower="${feat,,}"
		feat_enum="${feat_upper/FEAT_/CPU_FEATURE_}"
		feat_name="$(echo $feat_lower | cut -d_ -f2-)"
		feat_descr="$(echo "$line" | cut -d, -f2-)"
		echo -e "\t$feat_enum, /*!< ARM:$feat_descr */" >> "$TMP_LIBCPUID_H"
		echo -e "\t\t{ $feat_enum, \"$feat_name\" }," >> "$TMP_CPUID_MAIN_C"
	fi
done

echo "} cpu_feature_t;" >> "$TMP_LIBCPUID_H"
echo "	};" >> "$TMP_CPUID_MAIN_C"

echo "Extracted data for enum cpu_feature_t written to '$TMP_LIBCPUID_H' file."
echo "Extracted data for cpu_feature_str() matchtable written to '$TMP_CPUID_MAIN_C' file."
