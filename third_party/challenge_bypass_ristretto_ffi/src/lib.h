/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_CHALLENGE_BYPASS_RISTRETTO_FFI_SRC_LIB_H_
#define BRAVE_THIRD_PARTY_CHALLENGE_BYPASS_RISTRETTO_FFI_SRC_LIB_H_

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#if !defined(MERLIN_FEATURE_ENABLED)
/**
 * A `BatchDLEQProof` is a proof of the equivalence of the discrete logarithm
 * between a common pair of points and one or more other pairs of points.
 */
typedef struct C_BatchDLEQProof C_BatchDLEQProof;
#endif

#if defined(MERLIN_FEATURE_ENABLED)
/**
 * A `BatchDLEQProof` is a proof of the equivalence of the discrete logarithm
 * between a common pair of points and one or more other pairs of points.
 */
typedef struct C_BatchDLEQProof C_BatchDLEQProof;
#endif

/**
 * A `BlindedToken` is sent to the server for signing.
 * It is the result of the scalar multiplication of the point derived from the
 * token preimage with the blinding factor.
 * \\(P = T^r = H_1(t)^r\\)
 */
typedef struct C_BlindedToken C_BlindedToken;

#if !defined(MERLIN_FEATURE_ENABLED)
/**
 * A `DLEQProof` is a proof of the equivalence of the discrete logarithm between
 * two pairs of points.
 */
typedef struct C_DLEQProof C_DLEQProof;
#endif

#if defined(MERLIN_FEATURE_ENABLED)
/**
 * A `DLEQProof` is a proof of the equivalence of the discrete logarithm between
 * two pairs of points.
 */
typedef struct C_DLEQProof C_DLEQProof;
#endif

/**
 * A `PublicKey` is a committment by the server to a particular `SigningKey`.
 * \\(Y = X^k\\)
 */
typedef struct C_PublicKey C_PublicKey;

/**
 * A `SignedToken` is the result of signing a `BlindedToken`.
 * \\(Q = P^k = (T^r)^k\\)
 */
typedef struct C_SignedToken C_SignedToken;

/**
 * A `SigningKey` is used to sign a `BlindedToken` and verify an
 * `UnblindedToken`. This is a server secret and should NEVER be revealed to the
 * client.
 */
typedef struct C_SigningKey C_SigningKey;

/**
 * A `Token` consists of a randomly chosen preimage and blinding factor.
 * Since a token includes the blinding factor it should be treated
 * as a client secret and NEVER revealed to the server.
 */
typedef struct C_Token C_Token;

/**
 * A `TokenPreimage` is a slice of bytes which can be hashed to a
 * `RistrettoPoint`. The hash function must ensure the discrete log with respect
 * to other points is unknown. In this construction
 * `RistrettoPoint::from_uniform_bytes` is used as the hash function.
 */
typedef struct C_TokenPreimage C_TokenPreimage;

/**
 * An `UnblindedToken` is the result of unblinding a `SignedToken`.
 * While both the client and server both "know" this value,
 * it should nevertheless not be sent between the two.
 */
typedef struct C_UnblindedToken C_UnblindedToken;

/**
 * The shared `VerificationKey` for proving / verifying the validity of an
 * `UnblindedToken`.
 * \\(K = H_2(t, W)\\)
 */
typedef struct C_VerificationKey C_VerificationKey;

/**
 * A `VerificationSignature` which can be verified given the `VerificationKey`
 * and message
 */
typedef struct C_VerificationSignature C_VerificationSignature;

/**
 * Decode from base64 C string.
 * If something goes wrong, this will return a null pointer. Don't forget to
 * destroy the returned pointer once you are done with it!
 */
C_BatchDLEQProof* batch_dleq_proof_decode_base64(const uint8_t* s,
                                                 uintptr_t s_length);

/**
 * Destroy a `BatchDLEQProof` once you are done with it.
 */
void batch_dleq_proof_destroy(C_BatchDLEQProof* p);

/**
 * Return base64 encoding as a C string.
 */
char* batch_dleq_proof_encode_base64(const C_BatchDLEQProof* t);

/**
 * Check if a batch DLEQ proof is invalid
 * Returns -1 if an error was encountered, 1 if the proof failed verification
 * and 0 if valid NOTE this is named "invalid" instead of "verify" as it returns
 * true (non-zero) when the proof is invalid and false (zero) when valid
 */
int batch_dleq_proof_invalid(const C_BatchDLEQProof* proof,
                             const C_BlindedToken* const* blinded_tokens,
                             const C_SignedToken* const* signed_tokens,
                             int tokens_length,
                             const C_PublicKey* public_key);

/**
 * Check if a batch DLEQ proof is invalid and unblind each signed token if not
 * Returns -1 if an error was encountered, 1 if the proof failed verification
 * and 0 if valid NOTE this is named "invalid" instead of "verify" as it returns
 * true (non-zero) when the proof is invalid and false (zero) when valid
 */
int batch_dleq_proof_invalid_or_unblind(
    const C_BatchDLEQProof* proof,
    const C_Token* const* tokens,
    const C_BlindedToken* const* blinded_tokens,
    const C_SignedToken* const* signed_tokens,
    C_UnblindedToken** unblinded_tokens,
    int tokens_length,
    const C_PublicKey* public_key);

/**
 * Create a new batch DLEQ proof
 * If something goes wrong, this will return a null pointer. Don't forget to
 * destroy the `BatchDLEQProof` once you are done with it!
 */
C_BatchDLEQProof* batch_dleq_proof_new(
    const C_BlindedToken* const* blinded_tokens,
    const C_SignedToken* const* signed_tokens,
    int tokens_length,
    const C_SigningKey* key);

/**
 * Decode from base64 C string.
 * If something goes wrong, this will return a null pointer. Don't forget to
 * destroy the returned pointer once you are done with it!
 */
C_BlindedToken* blinded_token_decode_base64(const uint8_t* s,
                                            uintptr_t s_length);

/**
 * Destroy a `BlindedToken` once you are done with it.
 */
void blinded_token_destroy(C_BlindedToken* token);

/**
 * Return base64 encoding as a C string.
 */
char* blinded_token_encode_base64(const C_BlindedToken* t);

/**
 * Destroy a `*c_char` once you are done with it.
 */
void c_char_destroy(char* s);

/**
 * Decode from base64 C string.
 * If something goes wrong, this will return a null pointer. Don't forget to
 * destroy the returned pointer once you are done with it!
 */
C_DLEQProof* dleq_proof_decode_base64(const uint8_t* s, uintptr_t s_length);

/**
 * Destroy a `DLEQProof` once you are done with it.
 */
void dleq_proof_destroy(C_DLEQProof* p);

/**
 * Return base64 encoding as a C string.
 */
char* dleq_proof_encode_base64(const C_DLEQProof* t);

/**
 * Check if a DLEQ proof is invalid
 * Returns -1 if an error was encountered, 1 if the proof failed verification
 * and 0 if valid NOTE this is named "invalid" instead of "verify" as it returns
 * true (non-zero) when the proof is invalid and false (zero) when valid
 */
int dleq_proof_invalid(const C_DLEQProof* proof,
                       const C_BlindedToken* blinded_token,
                       const C_SignedToken* signed_token,
                       const C_PublicKey* public_key);

/**
 * Create a new DLEQ proof
 * If something goes wrong, this will return a null pointer. Don't forget to
 * destroy the `DLEQProof` once you are done with it!
 */
C_DLEQProof* dleq_proof_new(const C_BlindedToken* blinded_token,
                            const C_SignedToken* signed_token,
                            const C_SigningKey* key);

/**
 * Clear and return the message associated with the last error.
 */
char* last_error_message(void);

/**
 * Decode from base64 C string.
 * If something goes wrong, this will return a null pointer. Don't forget to
 * destroy the returned pointer once you are done with it!
 */
C_PublicKey* public_key_decode_base64(const uint8_t* s, uintptr_t s_length);

/**
 * Destroy a `PublicKey` once you are done with it.
 */
void public_key_destroy(C_PublicKey* k);

/**
 * Return base64 encoding as a C string.
 */
char* public_key_encode_base64(const C_PublicKey* t);

/**
 * Decode from base64 C string.
 * If something goes wrong, this will return a null pointer. Don't forget to
 * destroy the returned pointer once you are done with it!
 */
C_SignedToken* signed_token_decode_base64(const uint8_t* s, uintptr_t s_length);

/**
 * Destroy a `SignedToken` once you are done with it.
 */
void signed_token_destroy(C_SignedToken* token);

/**
 * Return base64 encoding as a C string.
 */
char* signed_token_encode_base64(const C_SignedToken* t);

/**
 * Decode from base64 C string.
 * If something goes wrong, this will return a null pointer. Don't forget to
 * destroy the returned pointer once you are done with it!
 */
C_SigningKey* signing_key_decode_base64(const uint8_t* s, uintptr_t s_length);

/**
 * Destroy a `SigningKey` once you are done with it.
 */
void signing_key_destroy(C_SigningKey* key);

/**
 * Return base64 encoding as a C string.
 */
char* signing_key_encode_base64(const C_SigningKey* t);

/**
 * Take a reference to a `SigningKey` and return it's associated `PublicKey`
 * If something goes wrong, this will return a null pointer. Don't forget to
 * destroy the `PublicKey` once you are done with it!
 */
C_PublicKey* signing_key_get_public_key(const C_SigningKey* key);

/**
 * Generate a new `SigningKey`
 * # Safety
 * Make sure you destroy the key with [`signing_key_destroy()`] once you are
 * done with it.
 */
C_SigningKey* signing_key_random(void);

/**
 * Take a reference to a `SigningKey` and use it to rederive an `UnblindedToken`
 * If something goes wrong, this will return a null pointer. Don't forget to
 * destroy the `UnblindedToken` once you are done with it!
 */
C_UnblindedToken* signing_key_rederive_unblinded_token(
    const C_SigningKey* key,
    const C_TokenPreimage* t);

/**
 * Take a reference to a `SigningKey` and use it to sign a `BlindedToken`,
 * returning a `SignedToken` If something goes wrong, this will return a null
 * pointer. Don't forget to destroy the `SignedToken` once you are done with it!
 */
C_SignedToken* signing_key_sign(const C_SigningKey* key,
                                const C_BlindedToken* token);

/**
 * Take a reference to a `Token` and blind it, returning a `BlindedToken`
 * If something goes wrong, this will return a null pointer. Don't forget to
 * destroy the `BlindedToken` once you are done with it!
 */
C_BlindedToken* token_blind(const C_Token* token);

/**
 * Decode from base64 C string.
 * If something goes wrong, this will return a null pointer. Don't forget to
 * destroy the returned pointer once you are done with it!
 */
C_Token* token_decode_base64(const uint8_t* s, uintptr_t s_length);

/**
 * Destroy a `Token` once you are done with it.
 */
void token_destroy(C_Token* token);

/**
 * Return base64 encoding as a C string.
 */
char* token_encode_base64(const C_Token* t);

/**
 * Decode from base64 C string.
 * If something goes wrong, this will return a null pointer. Don't forget to
 * destroy the returned pointer once you are done with it!
 */
C_TokenPreimage* token_preimage_decode_base64(const uint8_t* s,
                                              uintptr_t s_length);

/**
 * Destroy a `TokenPreimage` once you are done with it.
 */
void token_preimage_destroy(C_TokenPreimage* t);

/**
 * Return base64 encoding as a C string.
 */
char* token_preimage_encode_base64(const C_TokenPreimage* t);

/**
 * Generate a new `Token`
 * # Safety
 * Make sure you destroy the token with [`token_destroy()`] once you are
 * done with it.
 */
C_Token* token_random(void);

/**
 * Decode from base64 C string.
 * If something goes wrong, this will return a null pointer. Don't forget to
 * destroy the returned pointer once you are done with it!
 */
C_UnblindedToken* unblinded_token_decode_base64(const uint8_t* s,
                                                uintptr_t s_length);

/**
 * Take a reference to an `UnblindedToken` and use it to derive a
 * `VerificationKey` using Sha512 as the hash function If something goes wrong,
 * this will return a null pointer. Don't forget to destroy the
 * `VerificationKey` once you are done with it!
 */
C_VerificationKey* unblinded_token_derive_verification_key_sha512(
    const C_UnblindedToken* token);

/**
 * Destroy an `UnblindedToken` once you are done with it.
 */
void unblinded_token_destroy(C_UnblindedToken* token);

/**
 * Return base64 encoding as a C string.
 */
char* unblinded_token_encode_base64(const C_UnblindedToken* t);

/**
 * Take a reference to an `UnblindedToken` and return the corresponding
 * `TokenPreimage` If something goes wrong, this will return a null pointer.
 * Don't forget to destroy the `BlindedToken` once you are done with it!
 */
C_TokenPreimage* unblinded_token_preimage(const C_UnblindedToken* token);

/**
 * Destroy a `VerificationKey` once you are done with it.
 */
void verification_key_destroy(C_VerificationKey* key);

/**
 * Take a reference to a `VerificationKey` and use it to verify an
 * existing `VerificationSignature` using Sha512 as the HMAC hash function
 * Returns -1 if an error was encountered, 1 if the signature failed
 * verification and 0 if valid NOTE this is named "invalid" instead of "verify"
 * as it returns true (non-zero) when the signature is invalid and false (zero)
 * when valid
 */
int verification_key_invalid_sha512(const C_VerificationKey* key,
                                    const C_VerificationSignature* sig,
                                    const uint8_t* message,
                                    uintptr_t message_length);

/**
 * Take a reference to a `VerificationKey` and use it to sign a message
 * using Sha512 as the HMAC hash function to obtain a `VerificationSignature`
 * If something goes wrong, this will return a null pointer. Don't forget to
 * destroy the `VerificationSignature` once you are done with it!
 */
C_VerificationSignature* verification_key_sign_sha512(
    const C_VerificationKey* key,
    const uint8_t* message,
    uintptr_t message_length);

/**
 * Decode from base64 C string.
 * If something goes wrong, this will return a null pointer. Don't forget to
 * destroy the returned pointer once you are done with it!
 */
C_VerificationSignature* verification_signature_decode_base64(
    const uint8_t* s,
    uintptr_t s_length);

/**
 * Destroy a `VerificationSignature` once you are done with it.
 */
void verification_signature_destroy(C_VerificationSignature* sig);

/**
 * Return base64 encoding as a C string.
 */
char* verification_signature_encode_base64(const C_VerificationSignature* t);

#endif /* BRAVE_THIRD_PARTY_CHALLENGE_BYPASS_RISTRETTO_FFI_SRC_LIB_H_ */
