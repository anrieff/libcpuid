#!/bin/bash

print_help() {
	echo "$(basename "$0") NEW_VERSION SO_VERSION"
	echo -e "\n\033[1mNEW_VERSION format: MAJOR.MINOR.PATCH\033[0m"
	echo -e "\n\033[1mSO_VERSION format: C:A:R\033[0m"
	echo -e "Versioning scheme shamelessly copied from libexif."
	echo -e "Short walkthrough. C means CURRENT, A mean AGE, R means REVISION"
	echo -e "1) When you make a change that breaks binary compatibility, increment CURRENT, reset REVISION to 0 and:"
	echo -e "  1.1) If the change doesn't remove or change interfaces, increment AGE, otherwise reset to 0."
	echo -e "2) When you make just a source change that doesn't break binary compatibility, increment REVISION."
	echo -e "3) When you make a change that adds features, increment AGE, and set REVISION to 0"
}

if [[ $# -lt 2 ]]; then
	print_help
	exit 1
fi

GIT_DIR="$(git rev-parse --show-toplevel)"
OLD_VERSION="$(git describe --tags --abbrev=0 | sed 's/^v//')"
NEW_VERSION="$1"
SO_VERSION="$2"
DATE="$(date '+%Y-%m-%d')"

if ! [[ "$NEW_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
	echo "NEW_VERSION must contain dots (e.g. '0.6.2')"
	print_help
	exit 1
fi

if ! [[ "$SO_VERSION" =~ ^[0-9]+\:[0-9]+\:[0-9]+$ ]]; then
	echo "SO_VERSION must contain colons (e.g. '16:0:2')"
	print_help
	exit 1
fi

IFS=':' read -a so_version_array <<< "$SO_VERSION"
LIBCPUID_CURRENT="${so_version_array[0]}"
LIBCPUID_AGE="${so_version_array[1]}"
LIBCPUID_REVISION="${so_version_array[2]}"
echo "$DATE: bumping libcpuid version from $OLD_VERSION to $NEW_VERSION"
echo "LIBCPUID_CURRENT=$LIBCPUID_CURRENT LIBCPUID_AGE=$LIBCPUID_AGE LIBCPUID_REVISION=$LIBCPUID_REVISION"

echo -e "\nVersion $NEW_VERSION ($DATE):" >> "$GIT_DIR/ChangeLog"
sed -i "s|\[$OLD_VERSION\]|\[$NEW_VERSION\]|" "$GIT_DIR/configure.ac"
sed -i "s|LIBCPUID_CURRENT=.*|dnl $SO_VERSION   Version $NEW_VERSION:\nLIBCPUID_CURRENT=$LIBCPUID_CURRENT|" "$GIT_DIR/configure.ac"
sed -i "s|LIBCPUID_AGE=.*|LIBCPUID_AGE=$LIBCPUID_AGE|" "$GIT_DIR/configure.ac"
sed -i "s|LIBCPUID_REVISION=.*|LIBCPUID_REVISION=$LIBCPUID_REVISION|" "$GIT_DIR/configure.ac"
sed -i "s|VERSION \"$OLD_VERSION\"|VERSION \"$NEW_VERSION\"|" "$GIT_DIR/CMakeLists.txt"
sed -i "s|set(LIBCPUID_CURRENT .*)|set(LIBCPUID_CURRENT $LIBCPUID_CURRENT)|" "$GIT_DIR/CMakeLists.txt"
sed -i "s|set(LIBCPUID_AGE .*)|set(LIBCPUID_AGE $LIBCPUID_AGE)|" "$GIT_DIR/CMakeLists.txt"
sed -i "s|set(LIBCPUID_REVISION .*)|set(LIBCPUID_REVISION $LIBCPUID_REVISION)|" "$GIT_DIR/CMakeLists.txt"
sed -i "s|$OLD_VERSION|$NEW_VERSION|g" "$GIT_DIR/libcpuid/"{Makefile.x64,Makefile.x86,libcpuid.dsp,libcpuid_vc10.vcxproj,libcpuid_vc71.vcproj}
sed -i "s|\\\version  $OLD_VERSION|\\\version  $NEW_VERSION|" "$GIT_DIR/libcpuid/libcpuid.h"

echo "Model from https://github.com/anrieff/libcpuid/commit/a6123e8139ce73e26b5711c3e119b5b2bc9c9378"
echo -e "Also, make appropriate changes in follow files:\n- ChangeLog\n- configure.ac\n- libcpuid/libcpuid.h"
