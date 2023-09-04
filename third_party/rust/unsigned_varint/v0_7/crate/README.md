unsigned-varint encoding
========================

Unsigned varint encodes unsigned integers in 7-bit groups. The most
significant bit (MSB) in each byte indicates if another byte follows
(MSB = 1), or not (MSB = 0).

For details see: https://github.com/multiformats/unsigned-varint

