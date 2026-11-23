/*
 * Copyright (c) The mldsa-native project authors
 * SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT
 */

/* References
 * ==========
 *
 * - [FIPS204]
 *   FIPS 204 Module-Lattice-Based Digital Signature Standard
 *   National Institute of Standards and Technology
 *   https://csrc.nist.gov/pubs/fips/204/final
 */

#ifndef MLD_SIGN_H
#define MLD_SIGN_H

#include <stddef.h>
#include "cbmc.h"
#include "common.h"
#include "poly.h"
#include "polyvec.h"
#include "sys.h"

#if defined(MLD_CHECK_APIS)
/* Include to ensure consistency between internal sign.h
 * and external mldsa_native.h. */
#include "mldsa_native.h"

#if MLDSA_CRYPTO_SECRETKEYBYTES != \
    MLDSA_SECRETKEYBYTES(MLD_CONFIG_PARAMETER_SET)
#error Mismatch for SECRETKEYBYTES between sign.h and mldsa_native.h
#endif

#if MLDSA_CRYPTO_PUBLICKEYBYTES != \
    MLDSA_PUBLICKEYBYTES(MLD_CONFIG_PARAMETER_SET)
#error Mismatch for PUBLICKEYBYTES between sign.h and mldsa_native.h
#endif

#if MLDSA_CRYPTO_BYTES != MLDSA_BYTES(MLD_CONFIG_PARAMETER_SET)
#error Mismatch for BYTES between sign.h and mldsa_native.h
#endif

#endif /* MLD_CHECK_APIS */

#define mld_sign_keypair_internal \
  MLD_NAMESPACE_KL(keypair_internal) MLD_CONTEXT_PARAMETERS_3
#define mld_sign_keypair MLD_NAMESPACE_KL(keypair) MLD_CONTEXT_PARAMETERS_2
#define mld_sign_signature_internal \
  MLD_NAMESPACE_KL(signature_internal) MLD_CONTEXT_PARAMETERS_8
#define mld_sign_signature MLD_NAMESPACE_KL(signature) MLD_CONTEXT_PARAMETERS_6
#define mld_sign_signature_extmu \
  MLD_NAMESPACE_KL(signature_extmu) MLD_CONTEXT_PARAMETERS_3
#define mld_sign_verify_internal \
  MLD_NAMESPACE_KL(verify_internal) MLD_CONTEXT_PARAMETERS_7
#define mld_sign_verify MLD_NAMESPACE_KL(verify) MLD_CONTEXT_PARAMETERS_6
#define mld_sign_verify_extmu \
  MLD_NAMESPACE_KL(verify_extmu) MLD_CONTEXT_PARAMETERS_3
#define mld_sign_signature_pre_hash_internal \
  MLD_NAMESPACE_KL(signature_pre_hash_internal) MLD_CONTEXT_PARAMETERS_8
#define mld_sign_verify_pre_hash_internal \
  MLD_NAMESPACE_KL(verify_pre_hash_internal) MLD_CONTEXT_PARAMETERS_7
#define mld_sign_signature_pre_hash_shake256 \
  MLD_NAMESPACE_KL(signature_pre_hash_shake256) MLD_CONTEXT_PARAMETERS_7
#define mld_sign_verify_pre_hash_shake256 \
  MLD_NAMESPACE_KL(verify_pre_hash_shake256) MLD_CONTEXT_PARAMETERS_6
#define mld_prepare_domain_separation_prefix \
  MLD_NAMESPACE_KL(prepare_domain_separation_prefix)
#define mld_sign_pk_from_sk \
  MLD_NAMESPACE_KL(pk_from_sk) MLD_CONTEXT_PARAMETERS_2

/* Hash algorithm constants for domain separation */
#define MLD_PREHASH_NONE 0
#define MLD_PREHASH_SHA2_224 1
#define MLD_PREHASH_SHA2_256 2
#define MLD_PREHASH_SHA2_384 3
#define MLD_PREHASH_SHA2_512 4
#define MLD_PREHASH_SHA2_512_224 5
#define MLD_PREHASH_SHA2_512_256 6
#define MLD_PREHASH_SHA3_224 7
#define MLD_PREHASH_SHA3_256 8
#define MLD_PREHASH_SHA3_384 9
#define MLD_PREHASH_SHA3_512 10
#define MLD_PREHASH_SHAKE_128 11
#define MLD_PREHASH_SHAKE_256 12

#if !defined(MLD_CONFIG_NO_KEYPAIR_API)
/**
 * Generate a public-private key pair from a seed.
 *
 * When MLD_CONFIG_KEYGEN_PCT is set, performs a Pairwise Consistency Test
 * (PCT) as required by FIPS 140-3 IG.
 *
 * @spec{Implements @[FIPS204, Algorithm 6, ML-DSA.KeyGen_internal].}
 *
 * @param[out] pk      Output public key.
 * @param[out] sk      Output private key.
 * @param[in]  seed    Input random seed.
 * @param      context Application context. Only present when
 *                     MLD_CONFIG_CONTEXT_PARAMETER is defined; type set by
 *                     MLD_CONFIG_CONTEXT_PARAMETER_TYPE.
 *
 * @retval 0                               Success.
 * @retval MLD_ERR_OUT_OF_MEMORY           MLD_CONFIG_CUSTOM_ALLOC_FREE was
 *                                         used and an allocation via
 *                                         MLD_CUSTOM_ALLOC returned NULL.
 * @retval MLD_ERR_RNG_FAIL                Random number generation failed
 *                                         during the PCT. Only possible when
 *                                         MLD_CONFIG_KEYGEN_PCT is enabled.
 * @retval MLD_ERR_SIGNING_PAUSED          The PCT's signing step was paused by
 *                                         a MLD_CONFIG_SIGN_HOOK_ATTEMPT hook.
 *                                         This should currently never happen:
 *                                         signing hooks require
 *                                         MLD_CONFIG_NO_RANDOMIZED_API, which
 *                                         is incompatible with
 *                                         MLD_CONFIG_KEYGEN_PCT, so the two
 *                                         cannot be enabled simultaneously.
 * @retval MLD_ERR_PCT_FAIL                MLD_CONFIG_KEYGEN_PCT is enabled and
 *                                         the PCT check failed.
 */
MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
int mld_sign_keypair_internal(uint8_t pk[MLDSA_CRYPTO_PUBLICKEYBYTES],
                              uint8_t sk[MLDSA_CRYPTO_SECRETKEYBYTES],
                              const uint8_t seed[MLDSA_SEEDBYTES],
                              MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
__contract__(
  requires(memory_no_alias(pk, MLDSA_CRYPTO_PUBLICKEYBYTES))
  requires(memory_no_alias(sk, MLDSA_CRYPTO_SECRETKEYBYTES))
  requires(memory_no_alias(seed, MLDSA_SEEDBYTES))
  assigns(object_whole(pk))
  assigns(object_whole(sk))
  ensures(return_value == 0 || return_value == MLD_ERR_OUT_OF_MEMORY ||
          return_value == MLD_ERR_RNG_FAIL ||
          return_value == MLD_ERR_SIGNING_PAUSED ||
          return_value == MLD_ERR_PCT_FAIL)
  /* Output buffers on error, per API-CONVENTIONS.md */
  ensures(return_value != 0 ==>
          array_unchanged_or_zeroized_u8(pk, MLDSA_CRYPTO_PUBLICKEYBYTES))
  ensures(return_value != 0 ==>
          array_unchanged_or_zeroized_u8(sk, MLDSA_CRYPTO_SECRETKEYBYTES))
);

#if !defined(MLD_CONFIG_CORE_API_ONLY)
#if !defined(MLD_CONFIG_NO_RANDOMIZED_API)
/**
 * Generate a public-private key pair.
 *
 * When MLD_CONFIG_KEYGEN_PCT is set, performs a Pairwise Consistency Test
 * (PCT) as required by FIPS 140-3 IG.
 *
 * @spec{Implements @[FIPS204, Algorithm 1, ML-DSA.KeyGen].}
 *
 * @param[out] pk      Output public key.
 * @param[out] sk      Output private key.
 * @param      context Application context. Only present when
 *                     MLD_CONFIG_CONTEXT_PARAMETER is defined; type set by
 *                     MLD_CONFIG_CONTEXT_PARAMETER_TYPE.
 *
 * @retval 0                               Success.
 * @retval MLD_ERR_OUT_OF_MEMORY           MLD_CONFIG_CUSTOM_ALLOC_FREE was
 *                                         used and an allocation via
 *                                         MLD_CUSTOM_ALLOC returned NULL.
 * @retval MLD_ERR_RNG_FAIL                Random number generation failed.
 * @retval MLD_ERR_SIGNING_PAUSED          The PCT's signing step was paused by
 *                                         a MLD_CONFIG_SIGN_HOOK_ATTEMPT hook.
 *                                         This should currently never happen:
 *                                         signing hooks require
 *                                         MLD_CONFIG_NO_RANDOMIZED_API, which
 *                                         is incompatible with
 *                                         MLD_CONFIG_KEYGEN_PCT, so the two
 *                                         cannot be enabled simultaneously.
 * @retval MLD_ERR_PCT_FAIL                MLD_CONFIG_KEYGEN_PCT is enabled and
 *                                         the PCT check failed.
 */
MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
int mld_sign_keypair(uint8_t pk[MLDSA_CRYPTO_PUBLICKEYBYTES],
                     uint8_t sk[MLDSA_CRYPTO_SECRETKEYBYTES],
                     MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
__contract__(
  requires(memory_no_alias(pk, MLDSA_CRYPTO_PUBLICKEYBYTES))
  requires(memory_no_alias(sk, MLDSA_CRYPTO_SECRETKEYBYTES))
  assigns(object_whole(pk))
  assigns(object_whole(sk))
  ensures(return_value == 0 || return_value == MLD_ERR_OUT_OF_MEMORY ||
          return_value == MLD_ERR_RNG_FAIL ||
          return_value == MLD_ERR_SIGNING_PAUSED ||
          return_value == MLD_ERR_PCT_FAIL)
  /* Output buffers on error, per API-CONVENTIONS.md */
  ensures(return_value != 0 ==>
          array_unchanged_or_zeroized_u8(pk, MLDSA_CRYPTO_PUBLICKEYBYTES))
  ensures(return_value != 0 ==>
          array_unchanged_or_zeroized_u8(sk, MLDSA_CRYPTO_SECRETKEYBYTES))
);
#endif /* !MLD_CONFIG_NO_RANDOMIZED_API */
#endif /* !MLD_CONFIG_CORE_API_ONLY */
#endif /* !MLD_CONFIG_NO_KEYPAIR_API */

#if !defined(MLD_CONFIG_NO_SIGN_API)
/**
 * Compute signature using a caller-supplied random seed and prefix.
 *
 * On error (non-zero return value), the signature buffer sig is zeroized.
 *
 * @spec{Implements @[FIPS204, Algorithm 7, ML-DSA.Sign_internal].}
 *
 * @warning This function does not perform secret key validation.
 *          Callers importing serialized keys can use mld_sign_pk_from_sk
 *          to validate them before signing.
 *
 * @param[out] sig        Pointer to buffer to hold the generated signature of
 *                        MLDSA_CRYPTO_BYTES bytes.
 * @param[in]  m          Pointer to message to be signed (when
 *                        externalmu == 0), or to a precomputed
 *                        message representative mu (when externalmu != 0).
 * @param      mlen       Length of m. Must equal MLDSA_CRHBYTES when
 *                        externalmu != 0.
 * @param[in]  pre        Pointer to prefix string. Ignored when
 *                        externalmu != 0.
 * @param      prelen     Length of prefix string. Ignored when
 *                        externalmu != 0.
 * @param[in]  rnd        Random seed.
 * @param[in]  sk         Bit-packed secret key; assumed to be valid.
 * @param      externalmu 0: m/mlen is the raw message; mu = H(tr, pre, m) is
 *                        computed internally.
 *                        non-zero: m points to a precomputed mu of
 *                        MLDSA_CRHBYTES bytes; pre/prelen unused.
 * @param      context    Application context. Only present when
 *                        MLD_CONFIG_CONTEXT_PARAMETER is defined; type set by
 *                        MLD_CONFIG_CONTEXT_PARAMETER_TYPE.
 *
 * @retval 0                               Success.
 * @retval MLD_ERR_OUT_OF_MEMORY           MLD_CONFIG_CUSTOM_ALLOC_FREE was
 *                                         used and an allocation via
 *                                         MLD_CUSTOM_ALLOC returned NULL.
 * @retval MLD_ERR_SIGN_ATTEMPTS_EXHAUSTED The rejection-sampling loop exceeded
 *                                         MLD_CONFIG_MAX_SIGNING_ATTEMPTS
 *                                         iterations.
 * @retval MLD_ERR_SIGNING_PAUSED          A MLD_CONFIG_SIGN_HOOK_ATTEMPT hook
 *                                         paused signing; re-invoke to resume.
 */
MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
int mld_sign_signature_internal(uint8_t sig[MLDSA_CRYPTO_BYTES],
                                const uint8_t *m, size_t mlen,
                                const uint8_t *pre, size_t prelen,
                                const uint8_t rnd[MLDSA_RNDBYTES],
                                const uint8_t sk[MLDSA_CRYPTO_SECRETKEYBYTES],
                                int externalmu,
                                MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
__contract__(
  requires(mlen <= MLD_MAX_BUFFER_SIZE)
  requires(prelen <= MLD_MAX_BUFFER_SIZE)
  requires(memory_no_alias(sig, MLDSA_CRYPTO_BYTES))
  requires(memory_no_alias(m, mlen))
  requires(memory_no_alias(rnd, MLDSA_RNDBYTES))
  requires(memory_no_alias(sk, MLDSA_CRYPTO_SECRETKEYBYTES))
  requires((externalmu == 0) ==> ((prelen == 0) || memory_no_alias(pre, prelen)))
  requires((externalmu != 0) ==> (mlen == MLDSA_CRHBYTES))
  assigns(memory_slice(sig, MLDSA_CRYPTO_BYTES))
  ensures(return_value == 0 ||
          return_value == MLD_ERR_OUT_OF_MEMORY ||
          return_value == MLD_ERR_SIGN_ATTEMPTS_EXHAUSTED ||
          return_value == MLD_ERR_SIGNING_PAUSED)
  /* Output buffers on error, per API-CONVENTIONS.md */
  ensures(return_value != 0 ==>
          array_unchanged_or_zeroized_u8(sig, MLDSA_CRYPTO_BYTES)));

#if !defined(MLD_CONFIG_CORE_API_ONLY)
#if !defined(MLD_CONFIG_NO_RANDOMIZED_API)
/**
 * Compute signature. This function implements the randomized variant of
 * ML-DSA. If you require the deterministic variant, use
 * mld_sign_signature_internal directly.
 *
 * @spec{Implements @[FIPS204, Algorithm 2, ML-DSA.Sign].}
 *
 * @warning This function does not perform secret key validation.
 *          Callers importing serialized keys can use mld_sign_pk_from_sk
 *          to validate them before signing.
 *
 * @param[out] sig     Output signature.
 * @param[in]  m       Pointer to message to be signed.
 * @param      mlen    Length of message.
 * @param[in]  ctx     Pointer to context string. May be NULL if ctxlen == 0.
 * @param      ctxlen  Length of context string. Should be <= 255.
 * @param[in]  sk      Bit-packed secret key; assumed to be valid.
 * @param      context Application context. Only present when
 *                     MLD_CONFIG_CONTEXT_PARAMETER is defined; type set by
 *                     MLD_CONFIG_CONTEXT_PARAMETER_TYPE.
 *
 * @retval 0                               Success.
 * @retval MLD_ERR_OUT_OF_MEMORY           MLD_CONFIG_CUSTOM_ALLOC_FREE was
 *                                         used and an allocation via
 *                                         MLD_CUSTOM_ALLOC returned NULL.
 * @retval MLD_ERR_RNG_FAIL                Random number generation failed.
 * @retval MLD_ERR_SIGN_ATTEMPTS_EXHAUSTED The rejection-sampling loop exceeded
 *                                         MLD_CONFIG_MAX_SIGNING_ATTEMPTS
 *                                         iterations.
 * @retval MLD_ERR_SIGNING_PAUSED          A MLD_CONFIG_SIGN_HOOK_ATTEMPT hook
 *                                         paused signing; re-invoke to resume.
 * @retval MLD_ERR_INVALID_ARG             The context string exceeded 255
 *                                         bytes.
 */
MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
int mld_sign_signature(uint8_t sig[MLDSA_CRYPTO_BYTES], const uint8_t *m,
                       size_t mlen, const uint8_t *ctx, size_t ctxlen,
                       const uint8_t sk[MLDSA_CRYPTO_SECRETKEYBYTES],
                       MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
__contract__(
  requires(mlen <= MLD_MAX_BUFFER_SIZE)
  requires(memory_no_alias(sig, MLDSA_CRYPTO_BYTES))
  requires(memory_no_alias(m, mlen))
  requires(ctxlen <= MLD_MAX_BUFFER_SIZE)
  requires(ctxlen == 0 || memory_no_alias(ctx, ctxlen))
  requires(memory_no_alias(sk, MLDSA_CRYPTO_SECRETKEYBYTES))
  assigns(memory_slice(sig, MLDSA_CRYPTO_BYTES))
  ensures(return_value == 0 || return_value == MLD_ERR_OUT_OF_MEMORY ||
          return_value == MLD_ERR_RNG_FAIL ||
          return_value == MLD_ERR_SIGN_ATTEMPTS_EXHAUSTED ||
          return_value == MLD_ERR_SIGNING_PAUSED ||
          return_value == MLD_ERR_INVALID_ARG)
  ensures((return_value == MLD_ERR_INVALID_ARG) ==> (ctxlen > 255))
  /* Output buffers on error, per API-CONVENTIONS.md */
  ensures(return_value != 0 ==>
          array_unchanged_or_zeroized_u8(sig, MLDSA_CRYPTO_BYTES))
);

/**
 * Compute signature in "external mu" mode: the caller has already computed
 * the message representative mu = SHAKE256(tr || M', 64), where
 * tr = SHAKE256(pk, 64) and M' is the FIPS 204 formatted message (e.g.
 * 0x00 || ctxlen || ctx || msg for pure ML-DSA). This is the randomized
 * variant; for the deterministic variant, use mld_sign_signature_internal
 * directly with externalmu set to non-zero and an all-zero rnd.
 *
 * @spec{Implements @[FIPS204, Algorithm 2, ML-DSA.Sign external mu variant].}
 *
 * @warning This function does not perform secret key validation.
 *          Callers importing serialized keys can use mld_sign_pk_from_sk
 *          to validate them before signing.
 *
 * @param[out] sig     Output signature.
 * @param[in]  mu      Precomputed message representative.
 * @param[in]  sk      Bit-packed secret key; assumed to be valid.
 * @param      context Application context. Only present when
 *                     MLD_CONFIG_CONTEXT_PARAMETER is defined; type set by
 *                     MLD_CONFIG_CONTEXT_PARAMETER_TYPE.
 *
 * @retval 0                               Success.
 * @retval MLD_ERR_OUT_OF_MEMORY           MLD_CONFIG_CUSTOM_ALLOC_FREE was
 *                                         used and an allocation via
 *                                         MLD_CUSTOM_ALLOC returned NULL.
 * @retval MLD_ERR_RNG_FAIL                Random number generation failed.
 * @retval MLD_ERR_SIGN_ATTEMPTS_EXHAUSTED The rejection-sampling loop exceeded
 *                                         MLD_CONFIG_MAX_SIGNING_ATTEMPTS
 *                                         iterations.
 * @retval MLD_ERR_SIGNING_PAUSED          A MLD_CONFIG_SIGN_HOOK_ATTEMPT hook
 *                                         paused signing; re-invoke to resume.
 */
MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
int mld_sign_signature_extmu(uint8_t sig[MLDSA_CRYPTO_BYTES],
                             const uint8_t mu[MLDSA_CRHBYTES],
                             const uint8_t sk[MLDSA_CRYPTO_SECRETKEYBYTES],
                             MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
__contract__(
  requires(memory_no_alias(sig, MLDSA_CRYPTO_BYTES))
  requires(memory_no_alias(mu, MLDSA_CRHBYTES))
  requires(memory_no_alias(sk, MLDSA_CRYPTO_SECRETKEYBYTES))
  assigns(memory_slice(sig, MLDSA_CRYPTO_BYTES))
  ensures(return_value == 0 || return_value == MLD_ERR_OUT_OF_MEMORY ||
          return_value == MLD_ERR_RNG_FAIL ||
          return_value == MLD_ERR_SIGN_ATTEMPTS_EXHAUSTED ||
          return_value == MLD_ERR_SIGNING_PAUSED)
  /* Output buffers on error, per API-CONVENTIONS.md */
  ensures(return_value != 0 ==>
          array_unchanged_or_zeroized_u8(sig, MLDSA_CRYPTO_BYTES))
);

#endif /* !MLD_CONFIG_NO_RANDOMIZED_API */
#endif /* !MLD_CONFIG_CORE_API_ONLY */
#endif /* !MLD_CONFIG_NO_SIGN_API */

#if !defined(MLD_CONFIG_NO_VERIFY_API)
/**
 * Verify signature.
 *
 * @spec{Implements @[FIPS204, Algorithm 8, ML-DSA.Verify_internal].}
 *
 * @param[in] sig        Pointer to input signature of
 *                       MLDSA_CRYPTO_BYTES bytes.
 * @param[in] m          Pointer to message (when externalmu == 0), or to a
 *                       precomputed message representative mu (when
 *                       externalmu != 0).
 * @param     mlen       Length of m. Must equal MLDSA_CRHBYTES when
 *                       externalmu != 0.
 * @param[in] pre        Pointer to prefix string. Ignored when externalmu != 0.
 * @param     prelen     Length of prefix string. Ignored when externalmu != 0.
 * @param[in] pk         Bit-packed public key.
 * @param     externalmu 0: m/mlen is the raw message; mu = H(H(pk), pre, m) is
 *                       computed internally.
 *                       non-zero: m points to a precomputed mu of
 *                       MLDSA_CRHBYTES bytes; pre/prelen unused.
 * @param     context    Application context. Only present when
 *                       MLD_CONFIG_CONTEXT_PARAMETER is defined; type set by
 *                       MLD_CONFIG_CONTEXT_PARAMETER_TYPE.
 *
 * @retval 0                         Success.
 * @retval MLD_ERR_OUT_OF_MEMORY     MLD_CONFIG_CUSTOM_ALLOC_FREE was
 *                                   used and an allocation via
 *                                   MLD_CUSTOM_ALLOC returned NULL.
 * @retval MLD_ERR_INVALID_SIGNATURE Signature verification failed.
 */
MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
int mld_sign_verify_internal(const uint8_t sig[MLDSA_CRYPTO_BYTES],
                             const uint8_t *m, size_t mlen, const uint8_t *pre,
                             size_t prelen,
                             const uint8_t pk[MLDSA_CRYPTO_PUBLICKEYBYTES],
                             int externalmu,
                             MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
__contract__(
  requires(prelen <= MLD_MAX_BUFFER_SIZE)
  requires(mlen <= MLD_MAX_BUFFER_SIZE)
  requires(memory_no_alias(sig, MLDSA_CRYPTO_BYTES))
  requires(memory_no_alias(m, mlen))
  requires((externalmu == 0) ==> ((prelen == 0) || memory_no_alias(pre, prelen)))
  requires((externalmu != 0) ==> (mlen == MLDSA_CRHBYTES))
  requires(memory_no_alias(pk, MLDSA_CRYPTO_PUBLICKEYBYTES))
  ensures(return_value == 0 || return_value == MLD_ERR_INVALID_SIGNATURE || return_value == MLD_ERR_OUT_OF_MEMORY)
);

#if !defined(MLD_CONFIG_CORE_API_ONLY)
/**
 * Verify signature.
 *
 * @spec{Implements @[FIPS204, Algorithm 3, ML-DSA.Verify].}
 *
 * @param[in] sig     Pointer to input signature.
 * @param[in] m       Pointer to message.
 * @param     mlen    Length of message.
 * @param[in] ctx     Pointer to context string. May be NULL if ctxlen == 0.
 * @param     ctxlen  Length of context string.
 * @param[in] pk      Bit-packed public key.
 * @param     context Application context. Only present when
 *                    MLD_CONFIG_CONTEXT_PARAMETER is defined; type set by
 *                    MLD_CONFIG_CONTEXT_PARAMETER_TYPE.
 *
 * @retval 0                         Success.
 * @retval MLD_ERR_OUT_OF_MEMORY     MLD_CONFIG_CUSTOM_ALLOC_FREE was
 *                                   used and an allocation via
 *                                   MLD_CUSTOM_ALLOC returned NULL.
 * @retval MLD_ERR_INVALID_SIGNATURE Signature verification failed.
 * @retval MLD_ERR_INVALID_ARG       The context string exceeded 255 bytes.
 */
MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
int mld_sign_verify(const uint8_t sig[MLDSA_CRYPTO_BYTES], const uint8_t *m,
                    size_t mlen, const uint8_t *ctx, size_t ctxlen,
                    const uint8_t pk[MLDSA_CRYPTO_PUBLICKEYBYTES],
                    MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
__contract__(
  requires(mlen <= MLD_MAX_BUFFER_SIZE)
  requires(ctxlen <= MLD_MAX_BUFFER_SIZE)
  requires(memory_no_alias(sig, MLDSA_CRYPTO_BYTES))
  requires(memory_no_alias(m, mlen))
  requires(ctxlen == 0 || memory_no_alias(ctx, ctxlen))
  requires(memory_no_alias(pk, MLDSA_CRYPTO_PUBLICKEYBYTES))
  ensures(return_value == 0 || return_value == MLD_ERR_INVALID_SIGNATURE || return_value == MLD_ERR_INVALID_ARG || return_value == MLD_ERR_OUT_OF_MEMORY)
  ensures((return_value == MLD_ERR_INVALID_ARG) ==> (ctxlen > 255))
);

/**
 * Verify signature in "external mu" mode: the caller has already computed
 * the message representative mu = SHAKE256(tr || M', 64), where
 * tr = SHAKE256(pk, 64) and M' is the FIPS 204 formatted message (e.g.
 * 0x00 || ctxlen || ctx || msg for pure ML-DSA). The same mu must have been
 * used at signing time.
 *
 * @spec{Implements @[FIPS204, Algorithm 3, ML-DSA.Verify external mu variant].}
 *
 * @param[in] sig     Pointer to input signature.
 * @param[in] mu      Precomputed message representative.
 * @param[in] pk      Bit-packed public key.
 * @param     context Application context. Only present when
 *                    MLD_CONFIG_CONTEXT_PARAMETER is defined; type set by
 *                    MLD_CONFIG_CONTEXT_PARAMETER_TYPE.
 *
 * @retval 0                         Success.
 * @retval MLD_ERR_OUT_OF_MEMORY     MLD_CONFIG_CUSTOM_ALLOC_FREE was
 *                                   used and an allocation via
 *                                   MLD_CUSTOM_ALLOC returned NULL.
 * @retval MLD_ERR_INVALID_SIGNATURE Signature verification failed.
 */
MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
int mld_sign_verify_extmu(const uint8_t sig[MLDSA_CRYPTO_BYTES],
                          const uint8_t mu[MLDSA_CRHBYTES],
                          const uint8_t pk[MLDSA_CRYPTO_PUBLICKEYBYTES],
                          MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
__contract__(  requires(memory_no_alias(sig, MLDSA_CRYPTO_BYTES))
  requires(memory_no_alias(mu, MLDSA_CRHBYTES))
  requires(memory_no_alias(pk, MLDSA_CRYPTO_PUBLICKEYBYTES))
  ensures(return_value == 0 || return_value == MLD_ERR_INVALID_SIGNATURE || return_value == MLD_ERR_OUT_OF_MEMORY)
);

#endif /* !MLD_CONFIG_CORE_API_ONLY */
#endif /* !MLD_CONFIG_NO_VERIFY_API */

#if !defined(MLD_CONFIG_CORE_API_ONLY)
#if !defined(MLD_CONFIG_NO_SIGN_API)
/**
 * Compute signature with pre-hashed message.
 *
 * @spec{Implements @[FIPS204, Algorithm 4, HashML-DSA.Sign].}
 *
 * Supported hash algorithm constants:
 *   MLD_PREHASH_SHA2_224, MLD_PREHASH_SHA2_256, MLD_PREHASH_SHA2_384,
 *   MLD_PREHASH_SHA2_512, MLD_PREHASH_SHA2_512_224, MLD_PREHASH_SHA2_512_256,
 *   MLD_PREHASH_SHA3_224, MLD_PREHASH_SHA3_256, MLD_PREHASH_SHA3_384,
 *   MLD_PREHASH_SHA3_512, MLD_PREHASH_SHAKE_128, MLD_PREHASH_SHAKE_256.
 *
 * MLD_PREHASH_NONE is rejected by this API.
 *
 * @warning This is an unstable API that may change in the future. If you need
 * a stable API use mld_sign_signature_pre_hash_shake256.
 *
 * @warning This function does not perform secret key validation.
 *          Callers importing serialized keys can use mld_sign_pk_from_sk
 *          to validate them before signing.
 *
 * @param[out] sig     Output signature.
 * @param[in]  ph      Pointer to pre-hashed message.
 * @param      phlen   Length of pre-hashed message. Must match the output
 *                     length of hashalg (the digest size for SHA-2/SHA-3,
 *                     32 bytes for MLD_PREHASH_SHAKE_128, 64 bytes for
 *                     MLD_PREHASH_SHAKE_256).
 * @param[in]  ctx     Pointer to context string. May be NULL if ctxlen == 0.
 * @param      ctxlen  Length of context string.
 * @param[in]  rnd     Random seed.
 * @param[in]  sk      Bit-packed secret key; assumed to be valid.
 * @param      hashalg Hash algorithm constant (one of MLD_PREHASH_*).
 * @param      context Application context. Only present when
 *                     MLD_CONFIG_CONTEXT_PARAMETER is defined; type set by
 *                     MLD_CONFIG_CONTEXT_PARAMETER_TYPE.
 *
 * @retval 0                               Success.
 * @retval MLD_ERR_OUT_OF_MEMORY           MLD_CONFIG_CUSTOM_ALLOC_FREE was
 *                                         used and an allocation via
 *                                         MLD_CUSTOM_ALLOC returned NULL.
 * @retval MLD_ERR_SIGN_ATTEMPTS_EXHAUSTED The rejection-sampling loop exceeded
 *                                         MLD_CONFIG_MAX_SIGNING_ATTEMPTS
 *                                         iterations.
 * @retval MLD_ERR_SIGNING_PAUSED          A MLD_CONFIG_SIGN_HOOK_ATTEMPT hook
 *                                         paused signing; re-invoke to resume.
 * @retval MLD_ERR_INVALID_ARG             The pre-hash algorithm was
 *                                         MLD_PREHASH_NONE or unsupported,
 *                                         phlen did not match the output
 *                                         length of hashalg, or the context
 *                                         string exceeded 255 bytes.
 */
MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
int mld_sign_signature_pre_hash_internal(
    uint8_t sig[MLDSA_CRYPTO_BYTES], const uint8_t *ph, size_t phlen,
    const uint8_t *ctx, size_t ctxlen, const uint8_t rnd[MLDSA_RNDBYTES],
    const uint8_t sk[MLDSA_CRYPTO_SECRETKEYBYTES], int hashalg,
    MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
__contract__(
  requires(ctxlen <= MLD_MAX_BUFFER_SIZE)
  requires(phlen <= MLD_MAX_BUFFER_SIZE)
  requires(memory_no_alias(sig, MLDSA_CRYPTO_BYTES))
  requires(memory_no_alias(ph, phlen))
  requires(ctxlen == 0 || memory_no_alias(ctx, ctxlen))
  requires(memory_no_alias(rnd, MLDSA_RNDBYTES))
  requires(memory_no_alias(sk, MLDSA_CRYPTO_SECRETKEYBYTES))
  assigns(memory_slice(sig, MLDSA_CRYPTO_BYTES))
  ensures(return_value == 0 || return_value == MLD_ERR_OUT_OF_MEMORY || return_value == MLD_ERR_SIGN_ATTEMPTS_EXHAUSTED || return_value == MLD_ERR_SIGNING_PAUSED || return_value == MLD_ERR_INVALID_ARG)
  /* Output buffers on error, per API-CONVENTIONS.md */
  ensures(return_value != 0 ==>
          array_unchanged_or_zeroized_u8(sig, MLDSA_CRYPTO_BYTES))
);
#endif /* !MLD_CONFIG_NO_SIGN_API */

#if !defined(MLD_CONFIG_NO_VERIFY_API)
/**
 * Verify signature with pre-hashed message.
 *
 * @spec{Implements @[FIPS204, Algorithm 5, HashML-DSA.Verify].}
 *
 * Supported hash algorithm constants:
 *   MLD_PREHASH_SHA2_224, MLD_PREHASH_SHA2_256, MLD_PREHASH_SHA2_384,
 *   MLD_PREHASH_SHA2_512, MLD_PREHASH_SHA2_512_224, MLD_PREHASH_SHA2_512_256,
 *   MLD_PREHASH_SHA3_224, MLD_PREHASH_SHA3_256, MLD_PREHASH_SHA3_384,
 *   MLD_PREHASH_SHA3_512, MLD_PREHASH_SHAKE_128, MLD_PREHASH_SHAKE_256.
 *
 * MLD_PREHASH_NONE is rejected by this API.
 *
 * @warning This is an unstable API that may change in the future. If you need
 * a stable API use mld_sign_verify_pre_hash_shake256.
 *
 * @param[in] sig     Pointer to input signature.
 * @param[in] ph      Pointer to pre-hashed message.
 * @param     phlen   Length of pre-hashed message. Must match the output
 *                    length of hashalg (the digest size for SHA-2/SHA-3,
 *                    32 bytes for MLD_PREHASH_SHAKE_128, 64 bytes for
 *                    MLD_PREHASH_SHAKE_256).
 * @param[in] ctx     Pointer to context string. May be NULL if ctxlen == 0.
 * @param     ctxlen  Length of context string.
 * @param[in] pk      Bit-packed public key.
 * @param     hashalg Hash algorithm constant (one of MLD_PREHASH_*).
 * @param     context Application context. Only present when
 *                    MLD_CONFIG_CONTEXT_PARAMETER is defined; type set by
 *                    MLD_CONFIG_CONTEXT_PARAMETER_TYPE.
 *
 * @retval 0                         Success.
 * @retval MLD_ERR_OUT_OF_MEMORY     MLD_CONFIG_CUSTOM_ALLOC_FREE was
 *                                   used and an allocation via
 *                                   MLD_CUSTOM_ALLOC returned NULL.
 * @retval MLD_ERR_INVALID_SIGNATURE Signature verification failed.
 * @retval MLD_ERR_INVALID_ARG       The pre-hash algorithm was
 *                                   MLD_PREHASH_NONE or unsupported, phlen
 *                                   did not match the output length of
 *                                   hashalg, or the context string exceeded
 *                                   255 bytes.
 */
MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
int mld_sign_verify_pre_hash_internal(
    const uint8_t sig[MLDSA_CRYPTO_BYTES], const uint8_t *ph, size_t phlen,
    const uint8_t *ctx, size_t ctxlen,
    const uint8_t pk[MLDSA_CRYPTO_PUBLICKEYBYTES], int hashalg,
    MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
__contract__(
  requires(phlen <= MLD_MAX_BUFFER_SIZE)
  requires(ctxlen <= MLD_MAX_BUFFER_SIZE - 77)
  requires(memory_no_alias(sig, MLDSA_CRYPTO_BYTES))
  requires(memory_no_alias(ph, phlen))
  requires(ctxlen == 0 || memory_no_alias(ctx, ctxlen))
  requires(memory_no_alias(pk, MLDSA_CRYPTO_PUBLICKEYBYTES))
  ensures(return_value == 0 || return_value == MLD_ERR_INVALID_SIGNATURE || return_value == MLD_ERR_INVALID_ARG || return_value == MLD_ERR_OUT_OF_MEMORY)
);
#endif /* !MLD_CONFIG_NO_VERIFY_API */

#if !defined(MLD_CONFIG_NO_SIGN_API)
/**
 * Compute signature with pre-hashed message using SHAKE256. This function
 * computes the SHAKE256 hash of the message internally.
 *
 * @spec{Implements @[FIPS204, Algorithm 4, HashML-DSA.Sign] with SHAKE256 as
 * the pre-hash.}
 *
 * @warning This function does not perform secret key validation.
 *          Callers importing serialized keys can use mld_sign_pk_from_sk
 *          to validate them before signing.
 *
 * @param[out] sig     Output signature.
 * @param[in]  m       Pointer to message to be hashed and signed.
 * @param      mlen    Length of message.
 * @param[in]  ctx     Pointer to context string. May be NULL if ctxlen == 0.
 * @param      ctxlen  Length of context string.
 * @param[in]  rnd     Random seed.
 * @param[in]  sk      Bit-packed secret key; assumed to be valid.
 * @param      context Application context. Only present when
 *                     MLD_CONFIG_CONTEXT_PARAMETER is defined; type set by
 *                     MLD_CONFIG_CONTEXT_PARAMETER_TYPE.
 *
 * @retval 0                               Success.
 * @retval MLD_ERR_OUT_OF_MEMORY           MLD_CONFIG_CUSTOM_ALLOC_FREE was
 *                                         used and an allocation via
 *                                         MLD_CUSTOM_ALLOC returned NULL.
 * @retval MLD_ERR_SIGN_ATTEMPTS_EXHAUSTED The rejection-sampling loop exceeded
 *                                         MLD_CONFIG_MAX_SIGNING_ATTEMPTS
 *                                         iterations.
 * @retval MLD_ERR_SIGNING_PAUSED          A MLD_CONFIG_SIGN_HOOK_ATTEMPT hook
 *                                         paused signing; re-invoke to resume.
 * @retval MLD_ERR_INVALID_ARG             The context string exceeded 255
 *                                         bytes.
 */
MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
int mld_sign_signature_pre_hash_shake256(
    uint8_t sig[MLDSA_CRYPTO_BYTES], const uint8_t *m, size_t mlen,
    const uint8_t *ctx, size_t ctxlen, const uint8_t rnd[MLDSA_RNDBYTES],
    const uint8_t sk[MLDSA_CRYPTO_SECRETKEYBYTES],
    MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
__contract__(
  requires(mlen <= MLD_MAX_BUFFER_SIZE)
  requires(ctxlen <= MLD_MAX_BUFFER_SIZE)
  requires(memory_no_alias(sig, MLDSA_CRYPTO_BYTES))
  requires(memory_no_alias(m, mlen))
  requires(ctxlen == 0 || memory_no_alias(ctx, ctxlen))
  requires(memory_no_alias(rnd, MLDSA_RNDBYTES))
  requires(memory_no_alias(sk, MLDSA_CRYPTO_SECRETKEYBYTES))
  assigns(memory_slice(sig, MLDSA_CRYPTO_BYTES))
  ensures(return_value == 0 || return_value == MLD_ERR_OUT_OF_MEMORY || return_value == MLD_ERR_SIGN_ATTEMPTS_EXHAUSTED || return_value == MLD_ERR_SIGNING_PAUSED || return_value == MLD_ERR_INVALID_ARG)
  /* Output buffers on error, per API-CONVENTIONS.md */
  ensures(return_value != 0 ==>
          array_unchanged_or_zeroized_u8(sig, MLDSA_CRYPTO_BYTES))
);
#endif /* !MLD_CONFIG_NO_SIGN_API */

#if !defined(MLD_CONFIG_NO_VERIFY_API)
/**
 * Verify signature with pre-hashed message using SHAKE256. This function
 * computes the SHAKE256 hash of the message internally.
 *
 * @spec{Implements @[FIPS204, Algorithm 5, HashML-DSA.Verify] with SHAKE256 as
 * the pre-hash.}
 *
 * @param[in] sig     Pointer to input signature.
 * @param[in] m       Pointer to message to be hashed and verified.
 * @param     mlen    Length of message.
 * @param[in] ctx     Pointer to context string. May be NULL if ctxlen == 0.
 * @param     ctxlen  Length of context string.
 * @param[in] pk      Bit-packed public key.
 * @param     context Application context. Only present when
 *                    MLD_CONFIG_CONTEXT_PARAMETER is defined; type set by
 *                    MLD_CONFIG_CONTEXT_PARAMETER_TYPE.
 *
 * @retval 0                         Success.
 * @retval MLD_ERR_OUT_OF_MEMORY     MLD_CONFIG_CUSTOM_ALLOC_FREE was
 *                                   used and an allocation via
 *                                   MLD_CUSTOM_ALLOC returned NULL.
 * @retval MLD_ERR_INVALID_SIGNATURE Signature verification failed.
 * @retval MLD_ERR_INVALID_ARG       The context string exceeded 255 bytes.
 */
MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
int mld_sign_verify_pre_hash_shake256(
    const uint8_t sig[MLDSA_CRYPTO_BYTES], const uint8_t *m, size_t mlen,
    const uint8_t *ctx, size_t ctxlen,
    const uint8_t pk[MLDSA_CRYPTO_PUBLICKEYBYTES],
    MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
__contract__(
  requires(mlen <= MLD_MAX_BUFFER_SIZE)
  requires(ctxlen <= MLD_MAX_BUFFER_SIZE - 77)
  requires(memory_no_alias(sig, MLDSA_CRYPTO_BYTES))
  requires(memory_no_alias(m, mlen))
  requires(ctxlen == 0 || memory_no_alias(ctx, ctxlen))
  requires(memory_no_alias(pk, MLDSA_CRYPTO_PUBLICKEYBYTES))
  ensures(return_value == 0 || return_value == MLD_ERR_INVALID_SIGNATURE || return_value == MLD_ERR_INVALID_ARG || return_value == MLD_ERR_OUT_OF_MEMORY)
);
#endif /* !MLD_CONFIG_NO_VERIFY_API */

#if !defined(MLD_CONFIG_NO_SIGN_API) || !defined(MLD_CONFIG_NO_VERIFY_API)
/* Maximum formatted domain separation message length:
 * - Pure ML-DSA: 0x00 || ctxlen || ctx (max 255)
 * - HashML-DSA: 0x01 || ctxlen || ctx (max 255) || oid (11) || ph (max 64) */
#define MLD_DOMAIN_SEPARATION_MAX_BYTES (2 + 255 + 11 + 64)

/**
 * Prepare domain separation prefix for ML-DSA signing.
 *
 * For pure ML-DSA (hashalg == MLD_PREHASH_NONE):
 *   Format: 0x00 || ctxlen (1 byte) || ctx.
 *
 * For HashML-DSA (hashalg != MLD_PREHASH_NONE):
 *   Format: 0x01 || ctxlen (1 byte) || ctx || oid (11 bytes) || ph.
 *
 * This function is useful for building incremental signing APIs.
 *
 * @spec{For HashML-DSA (hashalg != MLD_PREHASH_NONE), implements
 * @[FIPS204, Algorithm 4, line 23]. For Pure ML-DSA
 * (hashalg == MLD_PREHASH_NONE), implements
 * ```
 *    M' <- BytesToBits(IntegerToBytes(0, 1)
 *           || IntegerToBytes(|ctx|, 1)
 *           || ctx
 * ```
 * which is part of @[FIPS204, Algorithm 2, ML-DSA.Sign, line 10] and
 * @[FIPS204, Algorithm 3, ML-DSA.Verify, line 5].}
 *
 * @param[out] prefix  Output domain separation prefix buffer.
 * @param[in]  ph      Pointer to pre-hashed message (ignored for pure
 *                     ML-DSA).
 * @param      phlen   Length of pre-hashed message; must match the output
 *                     length of hashalg (ignored for pure ML-DSA).
 * @param[in]  ctx     Pointer to context string. May be NULL if ctxlen == 0.
 * @param      ctxlen  Length of context string.
 * @param      hashalg Hash algorithm constant (MLD_PREHASH_NONE for pure
 *                     ML-DSA, or MLD_PREHASH_* for HashML-DSA).
 *
 * @return The total length of the formatted prefix, or 0 on error.
 *         Errors are:
 *         - The context string exceeded 255 bytes.
 *         - For HashML-DSA: hashalg was unsupported, ph was NULL, or phlen
 *           did not match the output length of hashalg.
 */
MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
size_t mld_prepare_domain_separation_prefix(
    uint8_t prefix[MLD_DOMAIN_SEPARATION_MAX_BYTES], const uint8_t *ph,
    size_t phlen, const uint8_t *ctx, size_t ctxlen, int hashalg)
__contract__(
  requires(ctxlen <= 255)
  requires(phlen <= MLD_MAX_BUFFER_SIZE)
  requires(ctxlen == 0 || memory_no_alias(ctx, ctxlen))
  requires(hashalg == MLD_PREHASH_NONE || memory_no_alias(ph, phlen))
  requires(memory_no_alias(prefix, MLD_DOMAIN_SEPARATION_MAX_BYTES))
  assigns(memory_slice(prefix, MLD_DOMAIN_SEPARATION_MAX_BYTES))
  ensures(return_value <= MLD_DOMAIN_SEPARATION_MAX_BYTES)
);
#endif /* !MLD_CONFIG_NO_SIGN_API || !MLD_CONFIG_NO_VERIFY_API */

#if !defined(MLD_CONFIG_NO_KEYPAIR_API)
/**
 * Perform basic validity checks on secret key, and derive public key.
 *
 * Referring to the decoding of the secret key `sk=(rho, K, tr, s1, s2, t0)`
 * (cf. @[FIPS204, Algorithm 25, skDecode]), the following checks are
 * performed:
 *   - Check that s1 and s2 have coefficients in [-MLDSA_ETA, MLDSA_ETA].
 *   - Check that t0 and tr stored in sk match recomputed values.
 *
 * @note This function leaks whether the secret key is valid or invalid
 * through its return value and timing.
 *
 * @param[out] pk      Output public key.
 * @param[in]  sk      Input secret key.
 * @param      context Application context. Only present when
 *                     MLD_CONFIG_CONTEXT_PARAMETER is defined; type set by
 *                     MLD_CONFIG_CONTEXT_PARAMETER_TYPE.
 *
 * @retval 0                    Success.
 * @retval MLD_ERR_OUT_OF_MEMORY MLD_CONFIG_CUSTOM_ALLOC_FREE was used and an
 *                               allocation via MLD_CUSTOM_ALLOC returned NULL.
 * @retval MLD_ERR_INVALID_KEY   Secret key validation failed.
 */
MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
int mld_sign_pk_from_sk(uint8_t pk[MLDSA_CRYPTO_PUBLICKEYBYTES],
                        const uint8_t sk[MLDSA_CRYPTO_SECRETKEYBYTES],
                        MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
__contract__(
  requires(memory_no_alias(pk, MLDSA_CRYPTO_PUBLICKEYBYTES))
  requires(memory_no_alias(sk, MLDSA_CRYPTO_SECRETKEYBYTES))
  assigns(memory_slice(pk, MLDSA_CRYPTO_PUBLICKEYBYTES))
  ensures(return_value == 0 || return_value == MLD_ERR_INVALID_KEY || return_value == MLD_ERR_OUT_OF_MEMORY)
  /* Output buffers on error, per API-CONVENTIONS.md */
  ensures(return_value != 0 ==>
          array_unchanged_or_zeroized_u8(pk, MLDSA_CRYPTO_PUBLICKEYBYTES))
);
#endif /* !MLD_CONFIG_NO_KEYPAIR_API */
#endif /* !MLD_CONFIG_CORE_API_ONLY */

#endif /* !MLD_SIGN_H */
