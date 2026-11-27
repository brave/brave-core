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

#ifndef MLD_H
#define MLD_H

/*
 * Public API for mldsa-native
 *
 * This header defines the public API of a single build of mldsa-native.
 *
 * Make sure the configuration file is in the include path
 * (this is "mldsa_native_config.h" by default, or MLD_CONFIG_FILE if defined).
 *
 * # API conventions
 *
 * Conventions shared by all functions below (return values, pointer validity,
 * output buffers on error) are documented in API-CONVENTIONS.md.
 *
 * # Multi-level builds
 *
 * This header specifies a build of mldsa-native for a fixed security level.
 * If you need multiple security levels, leave the security level unspecified
 * in the configuration file and include this header multiple times, setting
 * MLD_CONFIG_PARAMETER_SET accordingly for each, and #undef'ing the MLD_H
 * guard to allow multiple inclusions.
 */

/******************************* Key sizes ************************************/

/* Sizes of cryptographic material, per parameter set */
/* See mldsa/src/params.h for the arithmetic expressions giving rise to these */
/* check-magic: off */
#define MLDSA44_SECRETKEYBYTES 2560
#define MLDSA44_PUBLICKEYBYTES 1312
#define MLDSA44_BYTES 2420

#define MLDSA65_SECRETKEYBYTES 4032
#define MLDSA65_PUBLICKEYBYTES 1952
#define MLDSA65_BYTES 3309

#define MLDSA87_SECRETKEYBYTES 4896
#define MLDSA87_PUBLICKEYBYTES 2592
#define MLDSA87_BYTES 4627
/* check-magic: on */

/* Size of seed and randomness in bytes (level-independent) */
#define MLDSA_SEEDBYTES 32
#define MLDSA44_SEEDBYTES MLDSA_SEEDBYTES
#define MLDSA65_SEEDBYTES MLDSA_SEEDBYTES
#define MLDSA87_SEEDBYTES MLDSA_SEEDBYTES

/* Size of CRH output in bytes (level-independent) */
#define MLDSA_CRHBYTES 64
#define MLDSA44_CRHBYTES MLDSA_CRHBYTES
#define MLDSA65_CRHBYTES MLDSA_CRHBYTES
#define MLDSA87_CRHBYTES MLDSA_CRHBYTES

/* Size of TR in bytes (level-independent)
 *
 * TR = SHAKE256(pk, 64) is the hash of the public key. Callers of the
 * external-mu API (signature_extmu / verify_extmu) that compute the message
 * representative mu = SHAKE256(TR || M', MLDSA_CRHBYTES) themselves -- e.g. to
 * sign or verify a message that is too large or streamed to hold in memory --
 * need this constant to size the TR buffer. */
#define MLDSA_TRBYTES 64
#define MLDSA44_TRBYTES MLDSA_TRBYTES
#define MLDSA65_TRBYTES MLDSA_TRBYTES
#define MLDSA87_TRBYTES MLDSA_TRBYTES

/* Size of randomness for signing in bytes (level-independent) */
#define MLDSA_RNDBYTES 32
#define MLDSA44_RNDBYTES MLDSA_RNDBYTES
#define MLDSA65_RNDBYTES MLDSA_RNDBYTES
#define MLDSA87_RNDBYTES MLDSA_RNDBYTES

/* Sizes of cryptographic material, as a function of LVL=44,65,87 */
#define MLDSA_SECRETKEYBYTES_(LVL) MLDSA##LVL##_SECRETKEYBYTES
#define MLDSA_PUBLICKEYBYTES_(LVL) MLDSA##LVL##_PUBLICKEYBYTES
#define MLDSA_BYTES_(LVL) MLDSA##LVL##_BYTES
#define MLDSA_SECRETKEYBYTES(LVL) MLDSA_SECRETKEYBYTES_(LVL)
#define MLDSA_PUBLICKEYBYTES(LVL) MLDSA_PUBLICKEYBYTES_(LVL)
#define MLDSA_BYTES(LVL) MLDSA_BYTES_(LVL)

/****************************** Error codes ***********************************/

/* Generic failure condition, reserved for failures not covered by a more
 * specific error code. */
#define MLD_ERR_FAIL (-1)
/* An allocation failed. This can only happen if MLD_CONFIG_CUSTOM_ALLOC_FREE
 * is defined and the provided MLD_CUSTOM_ALLOC can fail. */
#define MLD_ERR_OUT_OF_MEMORY (-2)
/* An RNG failure occurred. Might be due to insufficient entropy or
 * system misconfiguration. */
#define MLD_ERR_RNG_FAIL (-3)
/* The signing rejection-sampling loop exceeded
 * MLD_CONFIG_MAX_SIGNING_ATTEMPTS iterations without producing a valid
 * signature. With a FIPS 204 Appendix C compliant bound (>= 821) this
 * has probability < 2^-256. */
#define MLD_ERR_SIGN_ATTEMPTS_EXHAUSTED (-4)
/* Signing was paused before completing, at the request of a caller-provided
 * MLD_CONFIG_SIGN_HOOK_ATTEMPT hook (see mldsa_native_config.h). The caller
 * resumes by re-invoking signing with the same inputs; the attempt hook,
 * together with MLD_CONFIG_SIGN_HOOK_RESUME, decides where to continue. */
#define MLD_ERR_SIGNING_PAUSED (-5)
/* Signature verification failed: the signature is not valid for the given
 * message and public key. Returned by the verification API. */
#define MLD_ERR_INVALID_SIGNATURE (-6)
/* Secret key validation failed: the secret key is malformed or internally
 * inconsistent. Returned by pk_from_sk. */
#define MLD_ERR_INVALID_KEY (-7)
/* The Pairwise Consistency Test failed. Only possible when
 * MLD_CONFIG_KEYGEN_PCT is enabled; signals that the freshly generated key
 * pair failed its sign/verify self-test. */
#define MLD_ERR_PCT_FAIL (-8)
/* An argument was invalid, e.g. an unsupported pre-hash algorithm or a context
 * string longer than 255 bytes. */
#define MLD_ERR_INVALID_ARG (-9)

/********************* Namespacing and Qualifiers *****************************/

#define MLD_API_CONCAT_(x, y) x##y
#define MLD_API_CONCAT(x, y) MLD_API_CONCAT_(x, y)
#define MLD_API_CONCAT_UNDERSCORE(x, y) MLD_API_CONCAT(MLD_API_CONCAT(x, _), y)

/* You need to make sure the config file is in the include path. */
#if defined(MLD_CONFIG_FILE)
#include MLD_CONFIG_FILE
#else
#include "mldsa_native_config.h"
#endif

/* Namespace prefix for the public API symbols. For multi-level builds, the
 * parameter set is appended to disambiguate the security levels. */
#if defined(MLD_CONFIG_MULTILEVEL_BUILD)
#define MLD_API_NAMESPACE_PREFIX \
  MLD_API_CONCAT(MLD_CONFIG_NAMESPACE_PREFIX, MLD_CONFIG_PARAMETER_SET)
#else
#define MLD_API_NAMESPACE_PREFIX MLD_CONFIG_NAMESPACE_PREFIX
#endif

#define MLD_API_NAMESPACE(sym) \
  MLD_API_CONCAT_UNDERSCORE(MLD_API_NAMESPACE_PREFIX, sym)

#if defined(__GNUC__) || defined(__clang__)
#define MLD_API_MUST_CHECK_RETURN_VALUE __attribute__((warn_unused_result))
#else
#define MLD_API_MUST_CHECK_RETURN_VALUE
#endif

#if defined(MLD_CONFIG_EXTERNAL_API_QUALIFIER)
#define MLD_API_QUALIFIER MLD_CONFIG_EXTERNAL_API_QUALIFIER
#else
#define MLD_API_QUALIFIER
#endif

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

/* Maximum formatted domain separation message length */
#define MLD_DOMAIN_SEPARATION_MAX_BYTES (2 + 255 + 11 + 64)

/****************************** Function API **********************************/

#if !defined(MLD_CONFIG_CONSTANTS_ONLY)

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#if !defined(MLD_CONFIG_NO_KEYPAIR_API)
/**
 * Generate a public-private key pair from a seed.
 *
 * When MLD_CONFIG_KEYGEN_PCT is set, performs a Pairwise Consistency Test
 * (PCT) as required by FIPS 140-3 IG.
 *
 * @warning The seed must be generated by a cryptographically secure random
 *          number generator.
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
MLD_API_QUALIFIER
MLD_API_MUST_CHECK_RETURN_VALUE
int MLD_API_NAMESPACE(keypair_internal)(
    uint8_t pk[MLDSA_PUBLICKEYBYTES(MLD_CONFIG_PARAMETER_SET)],
    uint8_t sk[MLDSA_SECRETKEYBYTES(MLD_CONFIG_PARAMETER_SET)],
    const uint8_t seed[MLDSA_SEEDBYTES]
#ifdef MLD_CONFIG_CONTEXT_PARAMETER
    ,
    MLD_CONFIG_CONTEXT_PARAMETER_TYPE context
#endif
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
MLD_API_QUALIFIER
MLD_API_MUST_CHECK_RETURN_VALUE
int MLD_API_NAMESPACE(keypair)(
    uint8_t pk[MLDSA_PUBLICKEYBYTES(MLD_CONFIG_PARAMETER_SET)],
    uint8_t sk[MLDSA_SECRETKEYBYTES(MLD_CONFIG_PARAMETER_SET)]
#ifdef MLD_CONFIG_CONTEXT_PARAMETER
    ,
    MLD_CONFIG_CONTEXT_PARAMETER_TYPE context
#endif
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
 *          Callers importing serialized keys can use pk_from_sk
 *          to validate them before signing.
 *
 * @param[out] sig        Pointer to buffer to hold the generated signature of
 *                        MLDSA_BYTES(MLD_CONFIG_PARAMETER_SET) bytes.
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
MLD_API_QUALIFIER
MLD_API_MUST_CHECK_RETURN_VALUE
int MLD_API_NAMESPACE(signature_internal)(
    uint8_t sig[MLDSA_BYTES(MLD_CONFIG_PARAMETER_SET)], const uint8_t *m,
    size_t mlen, const uint8_t *pre, size_t prelen,
    const uint8_t rnd[MLDSA_RNDBYTES],
    const uint8_t sk[MLDSA_SECRETKEYBYTES(MLD_CONFIG_PARAMETER_SET)],
    int externalmu
#ifdef MLD_CONFIG_CONTEXT_PARAMETER
    ,
    MLD_CONFIG_CONTEXT_PARAMETER_TYPE context
#endif
);

#if !defined(MLD_CONFIG_CORE_API_ONLY)
#if !defined(MLD_CONFIG_NO_RANDOMIZED_API)
/**
 * Compute signature. This function implements the randomized variant of
 * ML-DSA. If you require the deterministic variant, use
 * signature_internal directly.
 *
 * @spec{Implements @[FIPS204, Algorithm 2, ML-DSA.Sign].}
 *
 * @warning This function does not perform secret key validation.
 *          Callers importing serialized keys can use pk_from_sk
 *          to validate them before signing.
 *
 * @param[out] sig     Pointer to buffer to hold the generated signature of
 *                     MLDSA_BYTES(MLD_CONFIG_PARAMETER_SET) bytes.
 * @param[in]  m       Pointer to message to be signed. May be NULL if
 *                     mlen == 0.
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
MLD_API_QUALIFIER
MLD_API_MUST_CHECK_RETURN_VALUE
int MLD_API_NAMESPACE(signature)(
    uint8_t sig[MLDSA_BYTES(MLD_CONFIG_PARAMETER_SET)], const uint8_t *m,
    size_t mlen, const uint8_t *ctx, size_t ctxlen,
    const uint8_t sk[MLDSA_SECRETKEYBYTES(MLD_CONFIG_PARAMETER_SET)]
#ifdef MLD_CONFIG_CONTEXT_PARAMETER
    ,
    MLD_CONFIG_CONTEXT_PARAMETER_TYPE context
#endif
);

/**
 * Compute signature in "external mu" mode: the caller has already computed
 * the message representative mu = SHAKE256(tr || M', 64), where
 * tr = SHAKE256(pk, 64) and M' is the FIPS 204 formatted message (e.g.
 * 0x00 || ctxlen || ctx || msg for pure ML-DSA). This is useful when the
 * message is large or streamed and cannot be held in memory.
 *
 * @spec{Implements @[FIPS204, Algorithm 2, ML-DSA.Sign external mu variant].}
 *
 * @warning This function does not perform secret key validation.
 *          Callers importing serialized keys can use pk_from_sk
 *          to validate them before signing.
 *
 * @param[out] sig     Pointer to buffer to hold the generated signature of
 *                     MLDSA_BYTES(MLD_CONFIG_PARAMETER_SET) bytes.
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
MLD_API_QUALIFIER
MLD_API_MUST_CHECK_RETURN_VALUE
int MLD_API_NAMESPACE(signature_extmu)(
    uint8_t sig[MLDSA_BYTES(MLD_CONFIG_PARAMETER_SET)],
    const uint8_t mu[MLDSA_CRHBYTES],
    const uint8_t sk[MLDSA_SECRETKEYBYTES(MLD_CONFIG_PARAMETER_SET)]
#ifdef MLD_CONFIG_CONTEXT_PARAMETER
    ,
    MLD_CONFIG_CONTEXT_PARAMETER_TYPE context
#endif
);

#endif /* !MLD_CONFIG_NO_RANDOMIZED_API */
#endif /* !MLD_CONFIG_CORE_API_ONLY */
#endif /* !MLD_CONFIG_NO_SIGN_API */

#if !defined(MLD_CONFIG_NO_VERIFY_API)
/**
 * Verify signature. Internal API.
 *
 * @spec{Implements @[FIPS204, Algorithm 8, ML-DSA.Verify_internal].}
 *
 * @param[in] sig        Pointer to input signature of
 *                       MLDSA_BYTES(MLD_CONFIG_PARAMETER_SET) bytes.
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
MLD_API_QUALIFIER
MLD_API_MUST_CHECK_RETURN_VALUE
int MLD_API_NAMESPACE(verify_internal)(
    const uint8_t sig[MLDSA_BYTES(MLD_CONFIG_PARAMETER_SET)], const uint8_t *m,
    size_t mlen, const uint8_t *pre, size_t prelen,
    const uint8_t pk[MLDSA_PUBLICKEYBYTES(MLD_CONFIG_PARAMETER_SET)],
    int externalmu
#ifdef MLD_CONFIG_CONTEXT_PARAMETER
    ,
    MLD_CONFIG_CONTEXT_PARAMETER_TYPE context
#endif
);

#if !defined(MLD_CONFIG_CORE_API_ONLY)
/**
 * Verify signature.
 *
 * @spec{Implements @[FIPS204, Algorithm 3, ML-DSA.Verify].}
 *
 * @param[in] sig     Pointer to input signature of
 *                    MLDSA_BYTES(MLD_CONFIG_PARAMETER_SET) bytes.
 * @param[in] m       Pointer to message. May be NULL if mlen == 0.
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
MLD_API_QUALIFIER
MLD_API_MUST_CHECK_RETURN_VALUE
int MLD_API_NAMESPACE(verify)(
    const uint8_t sig[MLDSA_BYTES(MLD_CONFIG_PARAMETER_SET)], const uint8_t *m,
    size_t mlen, const uint8_t *ctx, size_t ctxlen,
    const uint8_t pk[MLDSA_PUBLICKEYBYTES(MLD_CONFIG_PARAMETER_SET)]
#ifdef MLD_CONFIG_CONTEXT_PARAMETER
    ,
    MLD_CONFIG_CONTEXT_PARAMETER_TYPE context
#endif
);

/**
 * Verify signature in "external mu" mode: the caller has already computed
 * the message representative mu = SHAKE256(tr || M', 64), where
 * tr = SHAKE256(pk, 64) and M' is the FIPS 204 formatted message (e.g.
 * 0x00 || ctxlen || ctx || msg for pure ML-DSA). The same mu must have
 * been used at signing time.
 *
 * @spec{Implements @[FIPS204, Algorithm 3, ML-DSA.Verify external mu variant].}
 *
 * @param[in] sig     Pointer to input signature of
 *                    MLDSA_BYTES(MLD_CONFIG_PARAMETER_SET) bytes.
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
MLD_API_QUALIFIER
MLD_API_MUST_CHECK_RETURN_VALUE
int MLD_API_NAMESPACE(verify_extmu)(
    const uint8_t sig[MLDSA_BYTES(MLD_CONFIG_PARAMETER_SET)],
    const uint8_t mu[MLDSA_CRHBYTES],
    const uint8_t pk[MLDSA_PUBLICKEYBYTES(MLD_CONFIG_PARAMETER_SET)]
#ifdef MLD_CONFIG_CONTEXT_PARAMETER
    ,
    MLD_CONFIG_CONTEXT_PARAMETER_TYPE context
#endif
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
 * a stable API use signature_pre_hash_shake256.
 *
 * @warning This function does not perform secret key validation.
 *          Callers importing serialized keys can use pk_from_sk
 *          to validate them before signing.
 *
 * @param[out] sig     Pointer to buffer to hold the generated signature of
 *                     MLDSA_BYTES(MLD_CONFIG_PARAMETER_SET) bytes.
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
MLD_API_QUALIFIER
MLD_API_MUST_CHECK_RETURN_VALUE
int MLD_API_NAMESPACE(signature_pre_hash_internal)(
    uint8_t sig[MLDSA_BYTES(MLD_CONFIG_PARAMETER_SET)], const uint8_t *ph,
    size_t phlen, const uint8_t *ctx, size_t ctxlen,
    const uint8_t rnd[MLDSA_RNDBYTES],
    const uint8_t sk[MLDSA_SECRETKEYBYTES(MLD_CONFIG_PARAMETER_SET)],
    int hashalg
#ifdef MLD_CONFIG_CONTEXT_PARAMETER
    ,
    MLD_CONFIG_CONTEXT_PARAMETER_TYPE context
#endif
);
#endif /* !MLD_CONFIG_NO_SIGN_API */

#if !defined(MLD_CONFIG_NO_VERIFY_API)
/**
 * Verifies signature with pre-hashed message.
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
 * a stable API use verify_pre_hash_shake256.
 *
 * @param[in] sig     Pointer to input signature of
 *                    MLDSA_BYTES(MLD_CONFIG_PARAMETER_SET) bytes.
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
MLD_API_QUALIFIER
MLD_API_MUST_CHECK_RETURN_VALUE
int MLD_API_NAMESPACE(verify_pre_hash_internal)(
    const uint8_t sig[MLDSA_BYTES(MLD_CONFIG_PARAMETER_SET)], const uint8_t *ph,
    size_t phlen, const uint8_t *ctx, size_t ctxlen,
    const uint8_t pk[MLDSA_PUBLICKEYBYTES(MLD_CONFIG_PARAMETER_SET)],
    int hashalg
#ifdef MLD_CONFIG_CONTEXT_PARAMETER
    ,
    MLD_CONFIG_CONTEXT_PARAMETER_TYPE context
#endif
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
 *          Callers importing serialized keys can use pk_from_sk
 *          to validate them before signing.
 *
 * @param[out] sig     Pointer to buffer to hold the generated signature of
 *                     MLDSA_BYTES(MLD_CONFIG_PARAMETER_SET) bytes.
 * @param[in]  m       Pointer to message to be hashed and signed. May be
 *                     NULL if mlen == 0.
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
MLD_API_QUALIFIER
MLD_API_MUST_CHECK_RETURN_VALUE
int MLD_API_NAMESPACE(signature_pre_hash_shake256)(
    uint8_t sig[MLDSA_BYTES(MLD_CONFIG_PARAMETER_SET)], const uint8_t *m,
    size_t mlen, const uint8_t *ctx, size_t ctxlen,
    const uint8_t rnd[MLDSA_RNDBYTES],
    const uint8_t sk[MLDSA_SECRETKEYBYTES(MLD_CONFIG_PARAMETER_SET)]
#ifdef MLD_CONFIG_CONTEXT_PARAMETER
    ,
    MLD_CONFIG_CONTEXT_PARAMETER_TYPE context
#endif
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
 * @param[in] sig     Pointer to input signature of
 *                    MLDSA_BYTES(MLD_CONFIG_PARAMETER_SET) bytes.
 * @param[in] m       Pointer to message to be hashed and verified. May be
 *                    NULL if mlen == 0.
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
MLD_API_QUALIFIER
MLD_API_MUST_CHECK_RETURN_VALUE
int MLD_API_NAMESPACE(verify_pre_hash_shake256)(
    const uint8_t sig[MLDSA_BYTES(MLD_CONFIG_PARAMETER_SET)], const uint8_t *m,
    size_t mlen, const uint8_t *ctx, size_t ctxlen,
    const uint8_t pk[MLDSA_PUBLICKEYBYTES(MLD_CONFIG_PARAMETER_SET)]
#ifdef MLD_CONFIG_CONTEXT_PARAMETER
    ,
    MLD_CONFIG_CONTEXT_PARAMETER_TYPE context
#endif
);
#endif /* !MLD_CONFIG_NO_VERIFY_API */
#endif /* !MLD_CONFIG_CORE_API_ONLY */

#if !defined(MLD_CONFIG_CORE_API_ONLY)
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
MLD_API_QUALIFIER
MLD_API_MUST_CHECK_RETURN_VALUE
size_t MLD_API_NAMESPACE(prepare_domain_separation_prefix)(
    uint8_t prefix[MLD_DOMAIN_SEPARATION_MAX_BYTES], const uint8_t *ph,
    size_t phlen, const uint8_t *ctx, size_t ctxlen, int hashalg);

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
MLD_API_QUALIFIER
MLD_API_MUST_CHECK_RETURN_VALUE
int MLD_API_NAMESPACE(pk_from_sk)(
    uint8_t pk[MLDSA_PUBLICKEYBYTES(MLD_CONFIG_PARAMETER_SET)],
    const uint8_t sk[MLDSA_SECRETKEYBYTES(MLD_CONFIG_PARAMETER_SET)]
#ifdef MLD_CONFIG_CONTEXT_PARAMETER
    ,
    MLD_CONFIG_CONTEXT_PARAMETER_TYPE context
#endif
);
#endif /* !MLD_CONFIG_NO_KEYPAIR_API */
#endif /* !MLD_CONFIG_CORE_API_ONLY */

#ifdef __cplusplus
}
#endif

#undef MLD_API_NAMESPACE_PREFIX

#endif /* !MLD_CONFIG_CONSTANTS_ONLY */

/***************************** Memory Usage **********************************/

/*
 * By default mldsa-native performs all memory allocations on the stack.
 * Alternatively, mldsa-native supports custom allocation of large structures
 * through the `MLD_CONFIG_CUSTOM_ALLOC_FREE` configuration option.
 * See mldsa_native_config.h for details.
 *
 * `MLD_TOTAL_ALLOC_{44,65,87}_{KEYPAIR,SIGN,VERIFY}` indicates the maximum
 * (accumulative) allocation via MLD_ALLOC for each parameter set and operation.
 * Note that some stack allocation remains even
 * when using custom allocators, so these values are lower than total stack
 * usage with the default stack-only allocation.
 *
 * These constants may be used to implement custom allocations using a
 * fixed-sized buffer and a simple allocator (e.g., bump allocator).
 */
/* check-magic: off */
#if !defined(MLD_CONFIG_REDUCE_RAM)
#define MLD_TOTAL_ALLOC_44_KEYPAIR_NO_PCT 26912
#define MLD_TOTAL_ALLOC_44_KEYPAIR_PCT 48480
#define MLD_TOTAL_ALLOC_44_PK_FROM_SK 28480
#define MLD_TOTAL_ALLOC_44_SIGN 44704
#define MLD_TOTAL_ALLOC_44_VERIFY 24448
#define MLD_TOTAL_ALLOC_65_KEYPAIR_NO_PCT 44320
#define MLD_TOTAL_ALLOC_65_KEYPAIR_PCT 74624
#define MLD_TOTAL_ALLOC_65_PK_FROM_SK 46720
#define MLD_TOTAL_ALLOC_65_SIGN 69312
#define MLD_TOTAL_ALLOC_65_VERIFY 39872
#define MLD_TOTAL_ALLOC_87_KEYPAIR_NO_PCT 75040
#define MLD_TOTAL_ALLOC_87_KEYPAIR_PCT 115488
#define MLD_TOTAL_ALLOC_87_PK_FROM_SK 78272
#define MLD_TOTAL_ALLOC_87_SIGN 108224
#define MLD_TOTAL_ALLOC_87_VERIFY 68800
#else /* !MLD_CONFIG_REDUCE_RAM */
#define MLD_TOTAL_ALLOC_44_KEYPAIR_NO_PCT 11584
#define MLD_TOTAL_ALLOC_44_KEYPAIR_PCT 16896
#define MLD_TOTAL_ALLOC_44_PK_FROM_SK 13152
#define MLD_TOTAL_ALLOC_44_SIGN 13120
#define MLD_TOTAL_ALLOC_44_VERIFY 9120
#define MLD_TOTAL_ALLOC_65_KEYPAIR_NO_PCT 14656
#define MLD_TOTAL_ALLOC_65_KEYPAIR_PCT 22560
#define MLD_TOTAL_ALLOC_65_PK_FROM_SK 17056
#define MLD_TOTAL_ALLOC_65_SIGN 17248
#define MLD_TOTAL_ALLOC_65_VERIFY 10208
#define MLD_TOTAL_ALLOC_87_KEYPAIR_NO_PCT 18752
#define MLD_TOTAL_ALLOC_87_KEYPAIR_PCT 28608
#define MLD_TOTAL_ALLOC_87_PK_FROM_SK 21984
#define MLD_TOTAL_ALLOC_87_SIGN 21344
#define MLD_TOTAL_ALLOC_87_VERIFY 12512
#endif /* MLD_CONFIG_REDUCE_RAM */
/* check-magic: on */

/*
 * MLD_TOTAL_ALLOC_*_KEYPAIR adapts based on MLD_CONFIG_KEYGEN_PCT.
 */
#if defined(MLD_CONFIG_KEYGEN_PCT)
#define MLD_TOTAL_ALLOC_44_KEYPAIR MLD_TOTAL_ALLOC_44_KEYPAIR_PCT
#define MLD_TOTAL_ALLOC_65_KEYPAIR MLD_TOTAL_ALLOC_65_KEYPAIR_PCT
#define MLD_TOTAL_ALLOC_87_KEYPAIR MLD_TOTAL_ALLOC_87_KEYPAIR_PCT
#else
#define MLD_TOTAL_ALLOC_44_KEYPAIR MLD_TOTAL_ALLOC_44_KEYPAIR_NO_PCT
#define MLD_TOTAL_ALLOC_65_KEYPAIR MLD_TOTAL_ALLOC_65_KEYPAIR_NO_PCT
#define MLD_TOTAL_ALLOC_87_KEYPAIR MLD_TOTAL_ALLOC_87_KEYPAIR_NO_PCT
#endif

#define MLD_MAX3_(a, b, c) \
  ((a) > (b) ? ((a) > (c) ? (a) : (c)) : ((b) > (c) ? (b) : (c)))
#define MLD_MAX4_(a, b, c, d) MLD_MAX3_((a), (b), MLD_MAX3_((c), (d), (d)))

/*
 * `MLD_TOTAL_ALLOC_{44,65,87}` is the maximum across standard API operations
 * (keygen, sign, verify) for each parameter set.
 */
#define MLD_TOTAL_ALLOC_44                                             \
  MLD_MAX4_(MLD_TOTAL_ALLOC_44_KEYPAIR, MLD_TOTAL_ALLOC_44_PK_FROM_SK, \
            MLD_TOTAL_ALLOC_44_SIGN, MLD_TOTAL_ALLOC_44_VERIFY)
#define MLD_TOTAL_ALLOC_65                                             \
  MLD_MAX4_(MLD_TOTAL_ALLOC_65_KEYPAIR, MLD_TOTAL_ALLOC_65_PK_FROM_SK, \
            MLD_TOTAL_ALLOC_65_SIGN, MLD_TOTAL_ALLOC_65_VERIFY)
#define MLD_TOTAL_ALLOC_87                                             \
  MLD_MAX4_(MLD_TOTAL_ALLOC_87_KEYPAIR, MLD_TOTAL_ALLOC_87_PK_FROM_SK, \
            MLD_TOTAL_ALLOC_87_SIGN, MLD_TOTAL_ALLOC_87_VERIFY)

#endif /* !MLD_H */
