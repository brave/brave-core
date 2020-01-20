
 tar cf - $1 -P | pv -s $(du -sb $1  | awk '{print $1}') | 7z a -si -m0=lzma2 -mx=3 $2.tar.7z
