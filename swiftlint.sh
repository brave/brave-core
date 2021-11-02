#!/bin/sh

#
# Runs SwiftLint to enforce style guides
#
# Adds support for Apple Silicon brew directory
export PATH="$PATH:/opt/homebrew/bin"

if which swiftlint; then
    swiftlint
else
  echo "Please install SwiftLint via Homebrew or directly from https://github.com/realm/SwiftLint"
  exit 1
fi

