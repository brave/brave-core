#!/bin/sh

#
# Runs SwiftLint to enforce style guides
#

command -v swiftlint >/dev/null 2>&1 || { echo >&2 "Please install SwiftLint via Homebrew or directly from https://github.com/realm/SwiftLint"; exit 1; }

cd $SRCROOT
swiftlint
