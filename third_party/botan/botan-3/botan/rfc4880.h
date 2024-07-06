/*
* (C) 2021 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_RFC4880_H_
#define BOTAN_RFC4880_H_

#include <botan/types.h>

namespace Botan {

/*
Helpers for encoding PGP S2K values (see RFC 4880)
*/

/**
* RFC 4880 encodes the iteration count to a single-byte value
*/
uint8_t BOTAN_PUBLIC_API(2, 8) RFC4880_encode_count(size_t iterations);

/**
* Decode the iteration count from RFC 4880 encoding
*/
size_t BOTAN_PUBLIC_API(2, 8) RFC4880_decode_count(uint8_t encoded_iter);

/**
* Round an arbitrary iteration count to next largest iteration count
* supported by RFC4880 encoding.
*/
inline size_t RFC4880_round_iterations(size_t iterations) {
   return RFC4880_decode_count(RFC4880_encode_count(iterations));
}

}  // namespace Botan

#endif
