#!/bin/bash
set -e # fail on any error
set -u # treat unset variables as error

# ARGUMENT $1 CARGO_TARGET
#Set additional dir path
if [ "${1}" == "" ]; then
  dir=""
else
  dir=".."
fi
mkdir -p ./artifacts/
cd ./target/$1/release/
ls -a
echo "_____ Find binary files in target _____"
find . -maxdepth 1 -type f ! -size 0 -exec grep -IL . "{}" \; | cut -c 3-
echo "_____ Move binaries to artifacts folder _____"
for binary in $(find . -maxdepth 1 -type f ! -size 0 -exec grep -IL . "{}" \; | cut -c 3- )
do
  mv -v $binary ../$dir/../artifacts/$binary
done
cd ../$dir/..
echo "_____ Clean target dir _____"
find ./target/$1/{debug,release} -maxdepth 1 -type f -delete;
rm -f  ./target/.rustc_info.json;
rm -rf ./target/$1/{debug,release}/{deps,.fingerprint}/
