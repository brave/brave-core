#!/bin/bash
set -e
pushd `dirname $0`

echo "Testing main blockers"

for file in *.json
do
  ./does-it-compile.swift "$file"
done

popd
