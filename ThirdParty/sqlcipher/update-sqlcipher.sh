#!/bin/bash

set -e

usage() {
  echo "./update-sqlcipher.sh [version]"
  echo "    version: A tag or branch name that exists on the sqlcipher repo"
  exit
}

[ "$#" -eq 1 ] || usage

if [ -d "/tmp/sqlcipher" ]; then
  rm -rf "/tmp/sqlcipher"
fi

git clone --depth 1 --branch "$1" https://github.com/sqlcipher/sqlcipher /tmp

OUTPUT_DIR="$(pwd)/sqlcipher/"

pushd /tmp/sqlcipher
./configure --with-crypto-lib=none
make sqlite3.c
cp sqlite3.c $OUTPUT_DIR
cp sqlite3.h $OUTPUT_DIR
cp sqlite3ext.h $OUTPUT_DIR
cp sqlite3session.h $OUTPUT_DIR
popd

rm -rf /tmp/sqlcipher

./create-xcframework.sh

echo $1 > VERSION
