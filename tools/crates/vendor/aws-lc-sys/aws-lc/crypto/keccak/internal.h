// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

#ifndef OPENSSL_HEADER_KECCAK_INTERNAL_H
#define OPENSSL_HEADER_KECCAK_INTERNAL_H

#include <openssl/base.h>

#include "../fipsmodule/sha/internal.h"

#if defined(__cplusplus)
extern "C" {
#endif

// Keccak-256 (Ethereum-style, original 0x01 padding).
//
// This is the pre-standardisation Keccak variant used by Ethereum and
// Solidity's keccak256(). It shares the rate, capacity and digest length of
// FIPS 202 SHA3-256 but uses the original Keccak padding byte (0x01) instead of
// the SHA3 domain-separation byte (0x06), so it produces different digests.
//
// Keccak-256 is NOT FIPS-approved and lives outside the FIPS module. It reuses
// the module's FIPS 202 buffering primitives (|KeccakSponge_Reset|,
// |KeccakSponge_Absorb|, |KeccakSponge_AbsorbFinal|) and the Keccak-f[1600]
// permutation, but sets up the context with |KECCAK256_PAD_CHAR| directly
// rather than going through the module's |KeccakSponge_Init| (which only
// accepts FIPS 202 padding). None of these functions touch the FIPS service
// indicator, so consumers always observe AWSLC_NOT_APPROVED. Public consumers
// reach Keccak-256 via |EVP_keccak256|.

// KECCAK256_PAD_CHAR is the original Keccak submission's padding byte, as used
// by Ethereum's keccak256. It is NOT the FIPS 202 padding byte.
#define KECCAK256_PAD_CHAR 0x01

#define KECCAK256_DIGEST_BITLENGTH 256
#define KECCAK256_DIGEST_LENGTH 32
#define KECCAK256_CBLOCK SHA3_BLOCKSIZE(KECCAK256_DIGEST_BITLENGTH)

// Keccak256_Init initialises |ctx| for a Keccak-256 computation and returns 1
// on success or 0 on failure.
int Keccak256_Init(KECCAK1600_CTX *ctx);

// Keccak256_Update absorbs |len| bytes from |data| into |ctx| and returns 1 on
// success or 0 on failure. It fails if |ctx| has already been finalised or was
// never initialised.
int Keccak256_Update(KECCAK1600_CTX *ctx, const void *data, size_t len);

// Keccak256_Final pads the last block and writes |KECCAK256_DIGEST_LENGTH|
// bytes to |out|. It returns 1 on success and 0 on programmer error. As with
// |SHA3_Final|, an uninitialised (zeroed) |ctx| returns 1 without writing to
// |out|.
int Keccak256_Final(uint8_t out[KECCAK256_DIGEST_LENGTH], KECCAK1600_CTX *ctx);

// Keccak256 writes the Keccak-256 digest of |len| bytes from |data| to |out|
// and returns |out| on success or NULL on failure. It is OPENSSL_EXPORTed so
// that in-tree test binaries, which link against the shared library, can call
// it directly; external users should access Keccak-256 via |EVP_keccak256|.
OPENSSL_EXPORT uint8_t *Keccak256(const uint8_t *data, size_t len,
                                  uint8_t out[KECCAK256_DIGEST_LENGTH]);

#if defined(__cplusplus)
}  // extern "C"
#endif

#endif  // OPENSSL_HEADER_KECCAK_INTERNAL_H
