# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# Creates an asset catalog in the provided output directory
#
# Usage: ./make_asset_catalog.sh icon[,icon1,icon2] -o outputDirectory

output_directory=""
leo_sf_symbols_directory=""
icons=()

function usage() {
  echo "Usage: ./ios_make_asset_catalog.sh -l leo_sf_symbols_directory -i icon[,icon1,icon2] -o output_directory"
  exit 1
}

while getopts ":l:o:i:" arg
do
  case "$arg" in
    o) output_directory="$OPTARG/LeoAssets.xcassets" ;;
    l) leo_sf_symbols_directory=$OPTARG ;;
    i) icons=($(echo "$OPTARG" | awk -F',' '{for(i=1; i<=NF; i++) print $i}')) ;;
  esac
done

if [ $output_directory = "" ] || [ $leo_sf_symbols_directory = "" ] || [ ${#icons[@]} -eq 0 ]; then
  usage
fi

echo "Output Directory: $output_directory"

if [ -d "$output_directory" ]; then
  rm -r "$output_directory"
fi

mkdir -p "$output_directory"
if [ ! -f "$output_directory/Contents.json" ]; then
cat > "$output_directory/Contents.json" << EOF
{
  "info" : {
    "author" : "xcode",
    "version" : 1
  }
}
EOF
fi

for icon in $icons
do
  declare svg_name="$icon.svg"
  if [ ! -f "$leo_sf_symbols_directory/symbols/$svg_name" ]; then
    echo "Could not find Leo SF symbol named $svg_name"
    exit 1
  fi
  mkdir -p "$output_directory/$icon.symbolset"
  cp "$leo_sf_symbols_directory/symbols/$svg_name" "$output_directory/$icon.symbolset/$svg_name"
  if [ ! -f "$output_directory/$icon.symbolset/Contents.json" ]; then
    cat > "$output_directory/$icon.symbolset/Contents.json" << EOF
{
  "info" : {
    "author" : "xcode",
    "version" : 1
  },
  "symbols" : [
    {
      "filename" : "$svg_name",
      "idiom" : "universal"
    }
  ]
}
EOF
  fi
done
