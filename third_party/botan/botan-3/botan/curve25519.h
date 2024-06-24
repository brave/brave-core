/*
* Curve25519
* (C) 2014 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_CURVE_25519_H_
#define BOTAN_CURVE_25519_H_

#include <botan/x25519.h>

BOTAN_DEPRECATED_HEADER("curve25519.h")

namespace Botan {

BOTAN_DEPRECATED("Use X25519_PublicKey") typedef X25519_PublicKey Curve25519_PublicKey;
BOTAN_DEPRECATED("Use X25519_PrivateKey") typedef X25519_PrivateKey Curve25519_PrivateKey;

}  // namespace Botan

#endif
