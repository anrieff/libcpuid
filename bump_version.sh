#!/bin/bash

if [[ $# -lt 2 ]]; then
	echo "$0 NEW_VERSION SO_VERSION"
	exit 1
fi

GIT_DIR="$(git rev-parse --show-toplevel)"
OLD_VERSION="$(git describe --tags --abbrev=0 | sed 's/^v//')"
NEW_VERSION="$1"
SO_VERSION="$2"
DATE="$(date '+%Y-%m-%d')"

if ! [[ "$SO_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
	echo "$0: SO_VERSION must contain dots (e.g. '15.0.1')"
	exit 1
fi

IFS='.' read -a so_version_array <<< "$SO_VERSION"
LIBCPUID_CURRENT="${so_version_array[0]}"
LIBCPUID_AGE="${so_version_array[1]}"
LIBCPUID_REVISION="${so_version_array[2]}"
echo "$DATE: bumping libcpuid version from $OLD_VERSION to $NEW_VERSION"
echo "LIBCPUID_CURRENT=$LIBCPUID_CURRENT LIBCPUID_AGE=$LIBCPUID_AGE LIBCPUID_REVISION=$LIBCPUID_REVISION"

echo -e "\nVersion $NEW_VERSION ($DATE):" >> "$GIT_DIR/ChangeLog"
sed -i "s|\[$OLD_VERSION\]|\[$NEW_VERSION\]|" "$GIT_DIR/configure.ac"
sed -i "s|LIBCPUID_CURRENT=.*|dnl $(echo $SO_VERSION | tr . :)   Version $NEW_VERSION:\nLIBCPUID_CURRENT=$LIBCPUID_CURRENT|" "$GIT_DIR/configure.ac"
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
