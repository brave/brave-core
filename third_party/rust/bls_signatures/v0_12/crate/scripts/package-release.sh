#!/usr/bin/env bash

if [ -z "$1" ]; then
  TAR_FILE=`mktemp`.tar.gz
else
  TAR_FILE=$1
fi

TAR_PATH=`mktemp -d`

mkdir -p $TAR_PATH
mkdir -p $TAR_PATH/include
mkdir -p $TAR_PATH/lib/pkgconfig

cp target/release/libbls_signatures.h $TAR_PATH/include/
cp target/release/libbls_signatures_ffi.a $TAR_PATH/lib/libbls_signatures.a
cp target/release/libbls_signatures.pc $TAR_PATH/lib/pkgconfig

pushd $TAR_PATH

tar -czf $TAR_FILE ./*

popd

rm -rf $TAR_PATH

echo $TAR_FILE
