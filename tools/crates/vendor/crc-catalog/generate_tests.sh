#!/bin/sh

echo "#![allow(non_snake_case)]"
echo "use crate::Algorithm;"

curl -s https://reveng.sourceforge.io/crc-catalogue/all.htm | grep -o 'width.*name.*"' | while read -r line; do
  # echo $(echo $line | \
  #   sed 's/ /, /g' | \
  #   sed 's/[-\/]/_/g' | \
  #   sed 's/width=\([0-9]*\), \(.*\), name="\(.*\)"/pub const \3: Algorithm<u\1> = Algorithm { \2 };/')
  
  width=$(echo $line | sed 's/width=\([0-9]*\) \(.*\) name="\(.*\)"/\1/')
  params=$(echo $line | sed 's/width=\([0-9]*\) \(.*\) name="\(.*\)"/\2/' | sed 's/ /, /g' | sed 's/=/: /g')
  name=$(echo $line | sed 's/width=\([0-9]*\) \(.*\) name="\(.*\)"/\3/' | sed 's/[-\/]/_/g')

  echo "#[test]"
  echo "fn test_$name() {"
  echo -n "    "
  if [ $width -le 8 ]; then
    echo "pub const $name: Algorithm<u8> = Algorithm { width: $width, $params };"
  elif [ $width -le 16 ]; then
    echo "pub const $name: Algorithm<u16> = Algorithm { width: $width, $params };"
  elif [ $width -le 32 ]; then
    echo "pub const $name: Algorithm<u32> = Algorithm { width: $width, $params };"
  elif [ $width -le 64 ]; then
    echo "pub const $name: Algorithm<u64> = Algorithm { width: $width, $params };"
  elif [ $width -le 128 ]; then
    echo "pub const $name: Algorithm<u128> = Algorithm { width: $width, $params };"
  fi
  echo "    assert_eq!($name, crate::algorithm::$name);"
  echo "}"
done
