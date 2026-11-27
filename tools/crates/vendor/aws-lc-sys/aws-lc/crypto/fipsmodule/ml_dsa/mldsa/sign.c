/*
 * Copyright (c) The mldsa-native project authors
 * SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT
 */

/* References
 * ==========
 *
 * - [FIPS140_3_IG]
 *   Implementation Guidance for FIPS 140-3 and the Cryptographic Module
 *   Validation Program
 *   National Institute of Standards and Technology
 *   https://csrc.nist.gov/projects/cryptographic-module-validation-program/fips-140-3-ig-announcements
 *
 * - [FIPS204]
 *   FIPS 204 Module-Lattice-Based Digital Signature Standard
 *   National Institute of Standards and Technology
 *   https://csrc.nist.gov/pubs/fips/204/final
 *
 * - [FIPS204_UPDATES]
 *   FIPS 204 Potential Updates (Errata)
 *   National Institute of Standards and Technology
 *   https://csrc.nist.gov/files/pubs/fips/204/final/docs/fips-204-potential-updates.xlsx
 *
 * - [Round3_Spec]
 *   CRYSTALS-Dilithium Algorithm Specifications and Supporting Documentation
 *   (Version 3.1)
 *   Bai, Ducas, Kiltz, Lepoint, Lyubashevsky, Schwabe, Seiler, Stehlé
 *   https://pq-crystals.org/dilithium/data/dilithium-specification-round3-20210208.pdf
 */

#include "sign.h"

#include "cbmc.h"
#include "ct.h"
#include "debug.h"
#include "packing.h"
#include "poly.h"
#include "poly_kl.h"
#include "polyvec.h"
#include "randombytes.h"
#include "symmetric.h"

/* Parameter set namespacing
 * This is to facilitate building multiple instances
 * of mldsa-native (e.g. with varying parameter sets)
 * within a single compilation unit. */
#define mld_check_pct MLD_ADD_PARAM_SET(mld_check_pct) MLD_CONTEXT_PARAMETERS_2
#define mld_sample_s1_s2 MLD_ADD_PARAM_SET(mld_sample_s1_s2)
#define mld_validate_hash_length MLD_ADD_PARAM_SET(mld_validate_hash_length)
#define mld_get_hash_oid MLD_ADD_PARAM_SET(mld_get_hash_oid)
#define mld_H MLD_ADD_PARAM_SET(mld_H)
#define mld_compute_pack_z MLD_ADD_PARAM_SET(mld_compute_pack_z)
#define mld_attempt_signature_generation \
  MLD_ADD_PARAM_SET(mld_attempt_signature_generation) MLD_CONTEXT_PARAMETERS_8
#define mld_compute_pack_t0_t1 \
  MLD_ADD_PARAM_SET(mld_compute_pack_t0_t1) MLD_CONTEXT_PARAMETERS_5
#define mld_get_max_signing_attempts \
  MLD_ADD_PARAM_SET(mld_get_max_signing_attempts)

#if !defined(MLD_CONFIG_NO_KEYPAIR_API)
static int mld_check_pct(uint8_t const pk[MLDSA_CRYPTO_PUBLICKEYBYTES],
                         uint8_t const sk[MLDSA_CRYPTO_SECRETKEYBYTES],
                         MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
__contract__(
  requires(memory_no_alias(pk, MLDSA_CRYPTO_PUBLICKEYBYTES))
  requires(memory_no_alias(sk, MLDSA_CRYPTO_SECRETKEYBYTES))
  ensures(return_value == 0 || return_value == MLD_ERR_OUT_OF_MEMORY ||
          return_value == MLD_ERR_RNG_FAIL ||
          return_value == MLD_ERR_SIGNING_PAUSED ||
          return_value == MLD_ERR_PCT_FAIL)
);

#if defined(MLD_CONFIG_KEYGEN_PCT)
/**
 * Pair-wise Consistency Test (PCT) for DSA keypairs.
 *
 * @[FIPS140_3_IG] TE10.35.02
 * (https://csrc.nist.gov/csrc/media/Projects/cryptographic-module-validation-program/documents/fips%20140-3/FIPS%20140-3%20IG.pdf).
 *
 * Validates that a generated public/private key pair can correctly sign and
 * verify data. Performs signature generation using the private key (sk),
 * followed by signature verification using the public key (pk).
 *
 * @note @[FIPS204] requires that public/private key pairs are to be used
 * only for the calculation and/or verification of digital signatures.
 *
 * @param[in] pk      Public key.
 * @param[in] sk      Secret key.
 * @param     context Application context. Only present when
 *                    MLD_CONFIG_CONTEXT_PARAMETER is defined; type set by
 *                    MLD_CONFIG_CONTEXT_PARAMETER_TYPE.
 *
 * @retval 0                      Success.
 * @retval MLD_ERR_OUT_OF_MEMORY  MLD_CONFIG_CUSTOM_ALLOC_FREE was used and an
 *                                allocation via MLD_CUSTOM_ALLOC returned NULL.
 * @retval MLD_ERR_RNG_FAIL       Random number generation failed.
 * @retval MLD_ERR_SIGNING_PAUSED The PCT's signing step was paused by a
 *                                MLD_CONFIG_SIGN_HOOK_ATTEMPT hook.
 * @retval MLD_ERR_PCT_FAIL       The consistency check failed.
 */
static int mld_check_pct(uint8_t const pk[MLDSA_CRYPTO_PUBLICKEYBYTES],
                         uint8_t const sk[MLDSA_CRYPTO_SECRETKEYBYTES],
                         MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
{
  MLD_ALIGN uint8_t message[1] = {0};
  int ret;
  MLD_ALLOC(signature, uint8_t, MLDSA_CRYPTO_BYTES, context);
  MLD_ALLOC(pk_test, uint8_t, MLDSA_CRYPTO_PUBLICKEYBYTES, context);

  if (signature == NULL || pk_test == NULL)
  {
    ret = MLD_ERR_OUT_OF_MEMORY;
    goto cleanup;
  }

  /* Copy public key for testing */
  mld_memcpy(pk_test, pk, MLDSA_CRYPTO_PUBLICKEYBYTES);

  /* Sign a test message using the original secret key */
  ret = mld_sign_signature(signature, message, sizeof(message), NULL, 0, sk,
                           context);
  if (ret != 0)
  {
    goto cleanup;
  }

#if defined(MLD_CONFIG_KEYGEN_PCT_BREAKAGE_TEST)
  /* Deliberately break public key for testing purposes */
  if (mld_break_pct())
  {
    pk_test[0] = ~pk_test[0];
  }
#endif /* MLD_CONFIG_KEYGEN_PCT_BREAKAGE_TEST */

  /* Verify the signature using the (potentially corrupted) public key. */
  ret = mld_sign_verify(signature, message, sizeof(message), NULL, 0, pk_test,
                        context);
  if (ret != 0)
  {
    goto cleanup;
  }

cleanup:
  /* @[FIPS204, Section 3.6.3] Destruction of intermediate values. */
  MLD_FREE(pk_test, uint8_t, MLDSA_CRYPTO_PUBLICKEYBYTES, context);
  MLD_FREE(signature, uint8_t, MLDSA_CRYPTO_BYTES, context);

  /* A failed signing operation or an invalid signature hint at a faulty
   * implementation and map to a dedicated error code for PCT failure. */
  if (ret == MLD_ERR_INVALID_SIGNATURE ||
      ret == MLD_ERR_SIGN_ATTEMPTS_EXHAUSTED)
  {
    ret = MLD_ERR_PCT_FAIL;
  }

  /* Other error codes, e.g. platform failures like out of memory or
   * randomness failure, are passed on unmodified. */

  return ret;
}
#else  /* MLD_CONFIG_KEYGEN_PCT */
static int mld_check_pct(uint8_t const pk[MLDSA_CRYPTO_PUBLICKEYBYTES],
                         uint8_t const sk[MLDSA_CRYPTO_SECRETKEYBYTES],
                         MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
{
  /* Skip PCT */
  ((void)pk);
  ((void)sk);
  MLD_CONTEXT_UNUSED(context);
  return 0;
}
#endif /* !MLD_CONFIG_KEYGEN_PCT */

/**
 * Sample the short secret vectors s1 (length MLDSA_L) and s2 (length MLDSA_K)
 * with coefficients in [-MLDSA_ETA, MLDSA_ETA] from the seed.
 *
 * @spec{Implements @[FIPS204, Algorithm 33, ExpandS].}
 *
 * @param[out] s1   Output vector s1.
 * @param[out] s2   Output vector s2.
 * @param[in]  seed Byte array with seed of length MLDSA_CRHBYTES.
 */
static void mld_sample_s1_s2(mld_polyvecl *s1, mld_polyveck *s2,
                             const uint8_t seed[MLDSA_CRHBYTES])
__contract__(
  requires(memory_no_alias(s1, sizeof(mld_polyvecl)))
  requires(memory_no_alias(s2, sizeof(mld_polyveck)))
  requires(memory_no_alias(seed, MLDSA_CRHBYTES))
  assigns(object_whole(s1), object_whole(s2))
  ensures(forall(l0, 0, MLDSA_L, array_abs_bound(s1->vec[l0].coeffs, 0, MLDSA_N, MLDSA_ETA + 1)))
  ensures(forall(k0, 0, MLDSA_K, array_abs_bound(s2->vec[k0].coeffs, 0, MLDSA_N, MLDSA_ETA + 1)))
)
{
/* Sample short vectors s1 and s2 */
#if defined(MLD_CONFIG_SERIAL_FIPS202_ONLY)
  int i;
  uint16_t nonce = 0;
  /* Safety: The nonces are at most 14 (MLDSA_L + MLDSA_K - 1), and, hence, the
   * casts are safe. */
  for (i = 0; i < MLDSA_L; i++)
  {
    mld_poly_uniform_eta(&s1->vec[i], seed, (uint8_t)(nonce + i));
  }
  for (i = 0; i < MLDSA_K; i++)
  {
    mld_poly_uniform_eta(&s2->vec[i], seed, (uint8_t)(nonce + MLDSA_L + i));
  }
#else /* MLD_CONFIG_SERIAL_FIPS202_ONLY */
#if MLD_CONFIG_PARAMETER_SET == 44
  mld_poly_uniform_eta_4x(&s1->vec[0], &s1->vec[1], &s1->vec[2], &s1->vec[3],
                          seed, 0, 1, 2, 3);
  mld_poly_uniform_eta_4x(&s2->vec[0], &s2->vec[1], &s2->vec[2], &s2->vec[3],
                          seed, 4, 5, 6, 7);
#elif MLD_CONFIG_PARAMETER_SET == 65
  mld_poly_uniform_eta_4x(&s1->vec[0], &s1->vec[1], &s1->vec[2], &s1->vec[3],
                          seed, 0, 1, 2, 3);
  mld_poly_uniform_eta_4x(&s1->vec[4], &s2->vec[0], &s2->vec[1],
                          &s2->vec[2] /* irrelevant */, seed, 4, 5, 6,
                          0xFF /* irrelevant */);
  mld_poly_uniform_eta_4x(&s2->vec[2], &s2->vec[3], &s2->vec[4], &s2->vec[5],
                          seed, 7, 8, 9, 10);
#elif MLD_CONFIG_PARAMETER_SET == 87
  mld_poly_uniform_eta_4x(&s1->vec[0], &s1->vec[1], &s1->vec[2], &s1->vec[3],
                          seed, 0, 1, 2, 3);
  mld_poly_uniform_eta_4x(&s1->vec[4], &s1->vec[5], &s1->vec[6],
                          &s2->vec[0] /* irrelevant */, seed, 4, 5, 6,
                          0xFF /* irrelevant */);
  mld_poly_uniform_eta_4x(&s2->vec[0], &s2->vec[1], &s2->vec[2], &s2->vec[3],
                          seed, 7, 8, 9, 10);
  mld_poly_uniform_eta_4x(&s2->vec[4], &s2->vec[5], &s2->vec[6], &s2->vec[7],
                          seed, 11, 12, 13, 14);
#endif /* MLD_CONFIG_PARAMETER_SET == 87 */
#endif /* !MLD_CONFIG_SERIAL_FIPS202_ONLY */
}

/**
 * Compute t = A*s1hat + s2 row by row, decompose each row into t0[k] and
 * t1[k] via power2round, and bit-pack t1[k] into pk_t1 and t0[k] into the
 * t0_packed buffer. Used by both keygen and pk_from_sk.
 *
 * @spec{Partially implements @[FIPS204, Algorithm 22, pkEncode] (t1) and
 * @[FIPS204, Algorithm 24, skEncode] (t0).}
 *
 * @param[out] pk_t1     Output buffer for packed t1 (size
 *                       MLDSA_K * MLDSA_POLYT1_PACKEDBYTES; i.e. the t1
 *                       region of pk).
 * @param[out] t0_packed Output buffer for packed t0 (size
 *                       MLDSA_K * MLDSA_POLYT0_PACKEDBYTES).
 * @param[in]  s1hat     s1 in NTT domain.
 * @param[in]  s2        s2.
 * @param[in]  rho       Byte array containing seed rho.
 * @param      context   Application context. Only present when
 *                       MLD_CONFIG_CONTEXT_PARAMETER is defined; type set by
 *                       MLD_CONFIG_CONTEXT_PARAMETER_TYPE.
 *
 * @return - 0: Success.
 *         - MLD_ERR_OUT_OF_MEMORY: If MLD_CONFIG_CUSTOM_ALLOC_FREE is used and
 *           an allocation via MLD_CUSTOM_ALLOC returned NULL.
 */
MLD_MUST_CHECK_RETURN_VALUE
static int mld_compute_pack_t0_t1(
    uint8_t pk_t1[MLDSA_K * MLDSA_POLYT1_PACKEDBYTES],
    uint8_t t0_packed[MLDSA_K * MLDSA_POLYT0_PACKEDBYTES],
    const mld_polyvecl *s1hat, const mld_polyveck *s2,
    const uint8_t rho[MLDSA_SEEDBYTES],
    MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
__contract__(
  requires(memory_no_alias(pk_t1, MLDSA_K * MLDSA_POLYT1_PACKEDBYTES))
  requires(memory_no_alias(t0_packed, MLDSA_K * MLDSA_POLYT0_PACKEDBYTES))
  requires(memory_no_alias(s1hat, sizeof(mld_polyvecl)))
  requires(memory_no_alias(s2, sizeof(mld_polyveck)))
  requires(memory_no_alias(rho, MLDSA_SEEDBYTES))
  requires(forall(l1, 0, MLDSA_L,
    array_abs_bound(s1hat->vec[l1].coeffs, 0, MLDSA_N, MLD_NTT_BOUND)))
  requires(forall(k2, 0, MLDSA_K,
    array_bound(s2->vec[k2].coeffs, 0, MLDSA_N,
      MLD_POLYETA_UNPACK_LOWER_BOUND, MLDSA_ETA + 1)))
  assigns(memory_slice(pk_t1, MLDSA_K * MLDSA_POLYT1_PACKEDBYTES))
  assigns(memory_slice(t0_packed, MLDSA_K * MLDSA_POLYT0_PACKEDBYTES))
  ensures(return_value == 0 || return_value == MLD_ERR_OUT_OF_MEMORY))
{
  unsigned int k;
  int ret;
  MLD_ALLOC(mat, mld_polymat, 1, context);
  MLD_ALLOC(t0k, mld_poly, 1, context);
  MLD_ALLOC(t1k, mld_poly, 1, context);

  if (mat == NULL || t0k == NULL || t1k == NULL)
  {
    ret = MLD_ERR_OUT_OF_MEMORY;
    goto cleanup;
  }

  /* Expand matrix */
  mld_polyvec_matrix_expand(mat, rho);

  for (k = 0; k < MLDSA_K; k++)
  __loop__(
    assigns(k, memory_slice(pk_t1, MLDSA_K * MLDSA_POLYT1_PACKEDBYTES),
            memory_slice(t0_packed, MLDSA_K * MLDSA_POLYT0_PACKEDBYTES),
            memory_slice(t0k, sizeof(mld_poly)),
            memory_slice(t1k, sizeof(mld_poly))
            MLD_IF_REDUCE_RAM(, memory_slice(mat, sizeof(mld_polymat))))
    invariant(k <= MLDSA_K)
    decreases(MLDSA_K - k)
  )
  {
    /* t0k = (A * s1hat)_k in NTT domain */
    mld_polyvec_matrix_pointwise_montgomery_row(t0k, mat, s1hat, k);

    /* t0k = invNTT(t0k) */
    mld_poly_invntt_tomont(t0k);

    /* t0k += s2[k] */
    mld_poly_add(t0k, &s2->vec[k]);

    /* Reference: The following reduction is not present in the reference
     *            implementation. Omitting this reduction requires the output
     *            of the invntt to be small enough such that the addition of
     *            s2 does not result in absolute values >= MLDSA_Q. While our
     *            C, x86_64, and AArch64 invntt implementations produce small
     *            enough values for this to work out, it complicates the
     *            bounds reasoning. We instead add an additional reduction,
     *            and can consequently, relax the bounds requirements for the
     *            invntt.
     */
    mld_poly_reduce(t0k);

    /* Decompose into t1[k] and t0[k] (in place into t0k). */
    mld_poly_caddq(t0k);
    mld_poly_power2round(t1k, t0k, t0k);

    /* Pack t1[k] into pk and t0[k] into the t0 output buffer. */
    mld_polyt1_pack(pk_t1 + k * MLDSA_POLYT1_PACKEDBYTES, t1k);
    mld_polyt0_pack(t0_packed + k * MLDSA_POLYT0_PACKEDBYTES, t0k);
  }

  ret = 0;
cleanup:
  /* @[FIPS204, Section 3.6.3] Destruction of intermediate values. */
  MLD_FREE(t1k, mld_poly, 1, context);
  MLD_FREE(t0k, mld_poly, 1, context);
  MLD_FREE(mat, mld_polymat, 1, context);
  return ret;
}

MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
int mld_sign_keypair_internal(uint8_t pk[MLDSA_CRYPTO_PUBLICKEYBYTES],
                              uint8_t sk[MLDSA_CRYPTO_SECRETKEYBYTES],
                              const uint8_t seed[MLDSA_SEEDBYTES],
                              MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
{
  int ret;
  const uint8_t *rho, *rhoprime, *key;

  MLD_ALLOC(seedbuf, uint8_t, 2 * MLDSA_SEEDBYTES + MLDSA_CRHBYTES, context);
  MLD_ALLOC(inbuf, uint8_t, MLDSA_SEEDBYTES + 2, context);
  MLD_ALLOC(tr, uint8_t, MLDSA_TRBYTES, context);
  MLD_ALLOC(s1, mld_polyvecl, 1, context);
  MLD_ALLOC(s2, mld_polyveck, 1, context);

  if (seedbuf == NULL || inbuf == NULL || tr == NULL || s1 == NULL ||
      s2 == NULL)
  {
    ret = MLD_ERR_OUT_OF_MEMORY;
    goto cleanup;
  }

  /* Get randomness for rho, rhoprime and key */
  mld_memcpy(inbuf, seed, MLDSA_SEEDBYTES);
  inbuf[MLDSA_SEEDBYTES + 0] = MLDSA_K;
  inbuf[MLDSA_SEEDBYTES + 1] = MLDSA_L;
  mld_shake256(seedbuf, 2 * MLDSA_SEEDBYTES + MLDSA_CRHBYTES, inbuf,
               MLDSA_SEEDBYTES + 2);
  rho = seedbuf;
  rhoprime = rho + MLDSA_SEEDBYTES;
  key = rhoprime + MLDSA_CRHBYTES;

  /* Constant time: rho is part of the public key and, hence, public. */
  MLD_CT_TESTING_DECLASSIFY(rho, MLDSA_SEEDBYTES);

  /* Sample s1 and s2 */
  mld_sample_s1_s2(s1, s2, rhoprime);

  /* Pack s1 into sk before NTT */
  mld_pack_sk_s1(sk, s1);

  /* NTT s1 in place to use as s1hat */
  mld_polyvecl_ntt(s1);

  /* Pack rho into pk */
  mld_memcpy(pk + MLDSA_PK_RHO_OFFSET, rho, MLDSA_SEEDBYTES);

  /* Compute t = A*s1hat + s2 row by row, decompose into t1/t0, and pack
   * t1 into pk and t0 directly into the t0 region of sk. */
  ret = mld_compute_pack_t0_t1(pk + MLDSA_PK_T1_OFFSET, sk + MLDSA_SK_T0_OFFSET,
                               s1, s2, rho, context);
  if (ret != 0)
  {
    goto cleanup;
  }

  /* Compute tr = H(pk) */
  mld_shake256(tr, MLDSA_TRBYTES, pk, MLDSA_CRYPTO_PUBLICKEYBYTES);

  /* Pack remaining secret key components (s1 and t0 already packed) */
  mld_pack_sk_rho_key_tr_s2(sk, rho, tr, key, s2);

  /* Constant time: pk is the public key, inherently public data */
  MLD_CT_TESTING_DECLASSIFY(pk, MLDSA_CRYPTO_PUBLICKEYBYTES);

cleanup:
  /* @[FIPS204, Section 3.6.3] Destruction of intermediate values. */
  MLD_FREE(s2, mld_polyveck, 1, context);
  MLD_FREE(s1, mld_polyvecl, 1, context);
  MLD_FREE(tr, uint8_t, MLDSA_TRBYTES, context);
  MLD_FREE(inbuf, uint8_t, MLDSA_SEEDBYTES + 2, context);
  MLD_FREE(seedbuf, uint8_t, 2 * MLDSA_SEEDBYTES + MLDSA_CRHBYTES, context);

  /* Pairwise Consistency Test (PCT) @[FIPS140_3_IG, p.87] */
  /* Do this after freeing all temporaries. */
  if (ret == 0)
  {
    ret = mld_check_pct(pk, sk, context);
  }

  if (ret != 0)
  {
    /* Clear caller outputs on failure. */
    mld_zeroize(pk, MLDSA_CRYPTO_PUBLICKEYBYTES);
    mld_zeroize(sk, MLDSA_CRYPTO_SECRETKEYBYTES);
  }

  return ret;
}

#if !defined(MLD_CONFIG_CORE_API_ONLY)
#if !defined(MLD_CONFIG_NO_RANDOMIZED_API)
MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
int mld_sign_keypair(uint8_t pk[MLDSA_CRYPTO_PUBLICKEYBYTES],
                     uint8_t sk[MLDSA_CRYPTO_SECRETKEYBYTES],
                     MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
{
  int ret;
  MLD_ALLOC(seed, uint8_t, MLDSA_SEEDBYTES, context);

  if (seed == NULL)
  {
    ret = MLD_ERR_OUT_OF_MEMORY;
    goto cleanup;
  }

  if (mld_randombytes(seed, MLDSA_SEEDBYTES) != 0)
  {
    ret = MLD_ERR_RNG_FAIL;
    goto cleanup;
  }
  MLD_CT_TESTING_SECRET(seed, MLDSA_SEEDBYTES);
  ret = mld_sign_keypair_internal(pk, sk, seed, context);

cleanup:
  /* @[FIPS204, Section 3.6.3] Destruction of intermediate values. */
  MLD_FREE(seed, uint8_t, MLDSA_SEEDBYTES, context);
  return ret;
}
#endif /* !MLD_CONFIG_NO_RANDOMIZED_API */
#endif /* !MLD_CONFIG_CORE_API_ONLY */
#endif /* !MLD_CONFIG_NO_KEYPAIR_API */

#if !defined(MLD_CONFIG_NO_SIGN_API) || !defined(MLD_CONFIG_NO_VERIFY_API)
/**
 * Abstracts application of SHAKE256 to one, two or three blocks of data,
 * yielding a user-requested size of output.
 *
 * @param[out] out    Pointer to output.
 * @param      outlen Requested output length in bytes.
 * @param[in]  in1    Pointer to input block 1. Must NOT be NULL.
 * @param      in1len Length of input in1 in bytes.
 * @param[in]  in2    Pointer to input block 2. May be NULL if in2len == 0,
 *                    in which case this block is ignored.
 * @param      in2len Length of input in2 in bytes.
 * @param[in]  in3    Pointer to input block 3. May be NULL if in3len == 0,
 *                    in which case this block is ignored.
 * @param      in3len Length of input in3 in bytes.
 */
static void mld_H(uint8_t *out, size_t outlen, const uint8_t *in1,
                  size_t in1len, const uint8_t *in2, size_t in2len,
                  const uint8_t *in3, size_t in3len)
__contract__(
  requires(in1len <= MLD_MAX_BUFFER_SIZE)
  requires(in2len <= MLD_MAX_BUFFER_SIZE)
  requires(in3len <= MLD_MAX_BUFFER_SIZE)
  requires(outlen <= 8 * SHAKE256_RATE /* somewhat arbitrary bound */)
  requires(memory_no_alias(in1, in1len))
  requires(in2len == 0 || memory_no_alias(in2, in2len))
  requires(in3len == 0 || memory_no_alias(in3, in3len))
  requires(memory_no_alias(out, outlen))
  assigns(memory_slice(out, outlen))
)
{
  mld_shake256ctx state;
  mld_shake256_init(&state);
  mld_shake256_absorb(&state, in1, in1len);
  if (in2len != 0)
  {
    mld_shake256_absorb(&state, in2, in2len);
  }
  if (in3len != 0)
  {
    mld_shake256_absorb(&state, in3, in3len);
  }
  mld_shake256_finalize(&state);
  mld_shake256_squeeze(out, outlen, &state);
  mld_shake256_release(&state);

  /* @[FIPS204, Section 3.6.3] Destruction of intermediate values. */
  mld_zeroize(&state, sizeof(state));
}
#endif /* !MLD_CONFIG_NO_SIGN_API || !MLD_CONFIG_NO_VERIFY_API */

#if !defined(MLD_CONFIG_NO_SIGN_API)
/* MLD_MAX_KAPPA (see params.h) bounds the rejection-sampling counter kappa;
 * MLD_MAX_SIGNING_ATTEMPTS below turns that into a bound on attempts. */

/**
 * Compute z = y + s1*c, check that z has coefficients smaller than
 * MLDSA_GAMMA1 - MLDSA_BETA, and pack z into the signature buffer.
 *
 * @reference{This function is inlined into mld_sign_signature in the
 * reference implementation.}
 *
 * @param[in,out] sig   Output signature.
 * @param[in]     cp    Challenge polynomial.
 * @param[in]     s1hat Secret vector s1 in NTT domain.
 * @param[in]     y     Masking vector y (or seed in REDUCE_RAM mode).
 * @param[out]    z     Scratch polynomial for z computation.
 * @param[out]    tmp   Scratch polynomial.
 *
 * @return - 0: Success (z has coefficients smaller than
 *           MLDSA_GAMMA1 - MLDSA_BETA).
 *         - MLD_ERR_FAIL: z rejected (norm check failed).
 *         - MLD_ERR_OUT_OF_MEMORY: If MLD_CONFIG_CUSTOM_ALLOC_FREE is used and
 *           an allocation via MLD_CUSTOM_ALLOC returned NULL.
 */
MLD_MUST_CHECK_RETURN_VALUE
static int mld_compute_pack_z(uint8_t sig[MLDSA_CRYPTO_BYTES],
                              const mld_poly *cp, const mld_sk_s1hat *s1hat,
                              const mld_yvec *y, mld_poly *z, mld_poly *tmp)
__contract__(
  requires(memory_no_alias(sig, MLDSA_CRYPTO_BYTES))
  requires(memory_no_alias(cp, sizeof(mld_poly)))
  requires(memory_no_alias(s1hat, sizeof(mld_sk_s1hat)))
  requires(memory_no_alias(y, sizeof(mld_yvec)))
  requires(memory_no_alias(z, sizeof(mld_poly)))
  requires(memory_no_alias(tmp, sizeof(mld_poly)))
  requires(array_abs_bound(cp->coeffs, 0, MLDSA_N, MLD_NTT_BOUND))
  MLD_IF_NOT_REDUCE_RAM(
    requires(forall(k0, 0, MLDSA_L,
      array_bound(y->vec.vec[k0].coeffs, 0, MLDSA_N, -(MLDSA_GAMMA1 - 1), MLDSA_GAMMA1 + 1)))
    requires(forall(k1, 0, MLDSA_L, array_abs_bound(s1hat->vec.vec[k1].coeffs, 0, MLDSA_N, MLD_NTT_BOUND)))
  )
  MLD_IF_REDUCE_RAM(
    requires(memory_no_alias(s1hat->packed, MLDSA_L * MLDSA_POLYETA_PACKEDBYTES))
    requires(memory_no_alias(y->rhoprime, MLDSA_CRHBYTES))
    requires(y->kappa <= MLD_MAX_KAPPA)
  )
  assigns(memory_slice(sig, MLDSA_CRYPTO_BYTES))
  assigns(memory_slice(z, sizeof(mld_poly)))
  assigns(memory_slice(tmp, sizeof(mld_poly)))
  ensures(return_value == 0 || return_value == MLD_ERR_FAIL ||
          return_value == MLD_ERR_OUT_OF_MEMORY)
)
{
  unsigned int i;
  uint32_t z_invalid;
  for (i = 0; i < MLDSA_L; i++)
  __loop__(
    assigns(i, memory_slice(z, sizeof(mld_poly)),
            memory_slice(tmp, sizeof(mld_poly)),
            memory_slice(sig, MLDSA_CRYPTO_BYTES))
    invariant(i <= MLDSA_L)
    decreases(MLDSA_L - i)
  )
  {
    mld_sk_s1hat_get_poly(z, s1hat, i);
    mld_poly_pointwise_montgomery(z, cp);
    mld_poly_invntt_tomont(z);
    mld_yvec_get_poly(tmp, y, i);
    mld_poly_add(z, tmp);
    mld_poly_reduce(z);

    z_invalid = mld_poly_chknorm(z, MLDSA_GAMMA1 - MLDSA_BETA);
    /* Constant time: It is fine (and prohibitively expensive to avoid)
     * to leak the result of the norm check and which polynomial in z caused a
     * rejection. It would even be okay to leak which coefficient led to
     * rejection as the candidate signature will be discarded anyway.
     * See Section 5.5 of @[Round3_Spec]. */
    MLD_CT_TESTING_DECLASSIFY(&z_invalid, sizeof(uint32_t));
    if (z_invalid)
    {
      return MLD_ERR_FAIL; /* reject */
    }
    /* If z is valid, then its coefficients are bounded by
     * MLDSA_GAMMA1 - MLDSA_BETA. This will be needed below
     * to prove the pre-condition of pack_sig_z() */
    mld_assert_abs_bound(z, MLDSA_N, (MLDSA_GAMMA1 - MLDSA_BETA));

    /* After the norm check, the distribution of each coefficient of z is
     * independent of the secret key and it can, hence, be considered
     * public. It is, hence, okay to immediately pack it into the user-provided
     * signature buffer. */
    mld_pack_sig_z(sig, z, i);
  }
  return 0;
}

/* Effective bound on signing attempts: the configured bound
 * MLD_CONFIG_MAX_SIGNING_ATTEMPTS (see mldsa_native_config.h) if set, otherwise
 * the hard type-safety bound MLD_MAX_KAPPA / MLDSA_L (see MLD_MAX_KAPPA in
 * params.h). */
#if defined(MLD_CONFIG_MAX_SIGNING_ATTEMPTS)

#if !defined(MLD_ALLOW_NONCOMPLIANT_SIGNING_BOUND) && \
    MLD_CONFIG_MAX_SIGNING_ATTEMPTS < 821
#error Bad configuration: MLD_CONFIG_MAX_SIGNING_ATTEMPTS must be >= 821 for FIPS 204 compliance @[FIPS204, Appendix C] @[FIPS204_UPDATES]
#endif

#if MLD_CONFIG_MAX_SIGNING_ATTEMPTS < 1
#error Bad configuration: MLD_CONFIG_MAX_SIGNING_ATTEMPTS must be >= 1
#endif

#if MLD_CONFIG_MAX_SIGNING_ATTEMPTS > MLD_MAX_KAPPA / MLDSA_L
#error Bad configuration: MLD_CONFIG_MAX_SIGNING_ATTEMPTS exceeds the maximum allowed value.
#endif

#define MLD_MAX_SIGNING_ATTEMPTS MLD_CONFIG_MAX_SIGNING_ATTEMPTS
#else /* MLD_CONFIG_MAX_SIGNING_ATTEMPTS */
#define MLD_MAX_SIGNING_ATTEMPTS (MLD_MAX_KAPPA / MLDSA_L)
#endif /* !MLD_CONFIG_MAX_SIGNING_ATTEMPTS */

MLD_MUST_CHECK_RETURN_VALUE
static MLD_INLINE uint16_t mld_get_max_signing_attempts(void)
__contract__(
  ensures(return_value >= 1)
  ensures(return_value <= MLD_MAX_KAPPA / MLDSA_L)
)
{
  /* cassert(0) ensures CBMC uses the contract rather than inlining the body,
   * keeping proofs agnostic of the configured value. */
  cassert(0);
  return MLD_MAX_SIGNING_ATTEMPTS;
}

/**
 * Attempt to generate a single signature: one iteration of the
 * ML-DSA.Sign_internal rejection-sampling loop.
 *
 * @spec{Implements one iteration of the rejection-sampling loop body of
 * @[FIPS204, Algorithm 7, ML-DSA.Sign_internal] (lines 11-30) plus, on success,
 * the sigEncode step (line 33). The per-signature setup (Algorithm 7 lines 1-7:
 * skDecode, NTT of s1/s2/t0, ExpandA, and computation of mu and rhoprime) and
 * the loop itself (lines 8-10, 31-32) live in the caller
 * mld_sign_signature_internal; kappa is this iteration's counter, used to
 * sample y.}
 *
 * @reference{This code differs from the reference implementation in that it
 * factors out the core signature generation step into a distinct function
 * here in order to improve efficiency of CBMC proof.}
 *
 * @param[out] sig      Pointer to output signature.
 * @param[in]  mu       Pointer to message or hash of exactly MLDSA_CRHBYTES
 *                      bytes.
 * @param[in]  rhoprime Pointer to randomness seed.
 * @param      kappa    Counter for this iteration (= attempt*MLDSA_L).
 * @param[in]  mat      Expanded matrix.
 * @param[in]  s1hat    Secret vector s1 in NTT domain.
 * @param[in]  s2hat    Secret vector s2 in NTT domain.
 * @param[in]  t0hat    Vector t0 in NTT domain.
 * @param      context  Application context. Only present when
 *                      MLD_CONFIG_CONTEXT_PARAMETER is defined; type set by
 *                      MLD_CONFIG_CONTEXT_PARAMETER_TYPE.
 *
 * @return - 0: Signature generation succeeded.
 *         - MLD_ERR_FAIL: Signature rejected (norm check failed).
 *         - MLD_ERR_OUT_OF_MEMORY: If MLD_CONFIG_CUSTOM_ALLOC_FREE is used and
 *           an allocation via MLD_CUSTOM_ALLOC returned NULL.
 */
MLD_MUST_CHECK_RETURN_VALUE
/* NOLINTNEXTLINE(readability-function-cognitive-complexity) */
static int mld_attempt_signature_generation(
    uint8_t sig[MLDSA_CRYPTO_BYTES], const uint8_t *mu,
    const uint8_t rhoprime[MLDSA_CRHBYTES], uint16_t kappa, mld_polymat *mat,
    const mld_sk_s1hat *s1hat, const mld_sk_s2hat *s2hat,
    const mld_sk_t0hat *t0hat, MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
__contract__(
  requires(memory_no_alias(sig, MLDSA_CRYPTO_BYTES))
  requires(memory_no_alias(mu, MLDSA_CRHBYTES))
  requires(memory_no_alias(rhoprime, MLDSA_CRHBYTES))
  requires(memory_no_alias(mat, sizeof(mld_polymat)))
  requires(memory_no_alias(s1hat, sizeof(mld_sk_s1hat)))
  requires(memory_no_alias(s2hat, sizeof(mld_sk_s2hat)))
  requires(memory_no_alias(t0hat, sizeof(mld_sk_t0hat)))
  requires(kappa <= MLD_MAX_KAPPA)
  MLD_IF_NOT_REDUCE_RAM(
    requires(forall(k1, 0, MLDSA_K, forall(l1, 0, MLDSA_L,
                                           array_bound(mat->vec[k1].vec[l1].coeffs, 0, MLDSA_N, 0, MLDSA_Q))))
    requires(forall(k2, 0, MLDSA_K, array_abs_bound(t0hat->vec.vec[k2].coeffs, 0, MLDSA_N, MLD_NTT_BOUND)))
    requires(forall(k3, 0, MLDSA_L, array_abs_bound(s1hat->vec.vec[k3].coeffs, 0, MLDSA_N, MLD_NTT_BOUND)))
    requires(forall(k4, 0, MLDSA_K, array_abs_bound(s2hat->vec.vec[k4].coeffs, 0, MLDSA_N, MLD_NTT_BOUND)))
  )
  MLD_IF_REDUCE_RAM(
    requires(memory_no_alias(s1hat->packed, MLDSA_L * MLDSA_POLYETA_PACKEDBYTES))
    requires(memory_no_alias(s2hat->packed, MLDSA_K * MLDSA_POLYETA_PACKEDBYTES))
    requires(memory_no_alias(t0hat->packed, MLDSA_K * MLDSA_POLYT0_PACKEDBYTES))
  )
  assigns(memory_slice(sig, MLDSA_CRYPTO_BYTES))
  MLD_IF_REDUCE_RAM(
    assigns(memory_slice(mat, sizeof(mld_polymat)))
  )
  ensures(return_value == 0 || return_value == MLD_ERR_FAIL ||
          return_value == MLD_ERR_OUT_OF_MEMORY)
)
{
  unsigned int k;
  uint32_t w0_invalid, h_invalid;
  int ret;

  typedef union
  {
    mld_polyveck w1;
    mld_polyvecl tmp;
  } w1tmp_u;
  mld_polyveck *w1;
  mld_polyvecl *tmp;

  MLD_ALLOC(challenge_bytes, uint8_t, MLDSA_CTILDEBYTES, context);
  MLD_ALLOC(y, mld_yvec, 1, context);
  MLD_ALLOC(z, mld_poly, 1, context);
  MLD_ALLOC(w1tmp, w1tmp_u, 1, context);
  MLD_ALLOC(w0, mld_polyveck, 1, context);
  MLD_ALLOC(cp, mld_poly, 1, context);
  MLD_ALLOC(t, mld_poly, 1, context);

  if (challenge_bytes == NULL || y == NULL || z == NULL || w1tmp == NULL ||
      w0 == NULL || cp == NULL || t == NULL)
  {
    ret = MLD_ERR_OUT_OF_MEMORY;
    goto cleanup;
  }
  w1 = &w1tmp->w1;
  tmp = &w1tmp->tmp;

  /* @[FIPS204, Algorithm 7, line 11] y <- ExpandMask(rhoprime, kappa). */
  mld_yvec_init(y, rhoprime, kappa);

  /* @[FIPS204, Algorithm 7, line 12] w <- invNTT(A_hat o NTT(y)). This call
   * performs the whole line: it NTTs y, accumulates the pointwise product with
   * A_hat, and applies the inverse NTT. In REDUCE_RAM mode the y sampling is
   * fused into the same pass. */
  mld_polyvec_matrix_pointwise_montgomery_yvec(w0, mat, y, tmp);

  /* @[FIPS204, Algorithm 7, line 13] w1 <- HighBits(w), here together with the
   * low part: Decompose yields w = 2*GAMMA2*w1 + w0, keeping both w1 and w0
   * (w0 is reused below in the line-21/26 alternative, see further down). */
  mld_polyveck_caddq(w0);
  mld_polyveck_decompose(w1, w0);

  /* @[FIPS204, Algorithm 7, line 15] ctilde <- H(mu || w1Encode(w1), lambda/4).
   * w1Encode(w1) is packed into the w1 region of sig (mld_polyveck_pack_w1),
   * then absorbed by H together with mu. */
  mld_polyveck_pack_w1(sig, w1);

  mld_H(challenge_bytes, MLDSA_CTILDEBYTES, mu, MLDSA_CRHBYTES, sig,
        MLDSA_K * MLDSA_POLYW1_PACKEDBYTES, NULL, 0);
  /* Constant time: Leaking challenge_bytes does not reveal any information
   * about the secret key as H() is modelled as random oracle.
   * This also applies to challenges for rejected signatures.
   * See Section 5.5 of @[Round3_Spec]. */
  MLD_CT_TESTING_DECLASSIFY(challenge_bytes, MLDSA_CTILDEBYTES);
  /* @[FIPS204, Algorithm 7, line 16] c <- SampleInBall(ctilde) and
   * @[FIPS204, Algorithm 7, line 17] c_hat <- NTT(c). */
  mld_poly_challenge(cp, challenge_bytes);
  mld_poly_ntt(cp);

  /* @[FIPS204, Algorithm 7, lines 18+20] cs1 <- invNTT(c_hat o s1_hat) and
   * z <- y + cs1, followed by the line-23 norm check ||z||_inf >= GAMMA1 -
   * BETA. mld_compute_pack_z fuses all three per polynomial and, on success,
   * packs z into sig; it returns MLD_ERR_FAIL if the norm check rejects z. */
  ret = mld_compute_pack_z(sig, cp, s1hat, y, t, z);
  if (ret != 0)
  {
    goto cleanup;
  }

  /* The remaining steps realize @[FIPS204, Algorithm 7, lines 21-28] (the
   * low-bits norm check and the hint h) via the faster alternative formulation
   * of @[Round3_Spec, Section 5.1]. @[FIPS204] explicitly permits this: the
   * note accompanying Algorithm 7 states that the validity checks on z and the
   * computation of h may instead be implemented "as described in Section 5.1 of
   * [6]", and that reference is @[Round3_Spec, Section 5.1].
   *
   * The loop below builds w0 - cs2 + ct0 in place in w0; w1 is unmodified, and
   * is HighBits(w) from line 13. Those are the inputs to the streamlined
   * computation of MakeHint explained below.
   *
   * Low-bits norm check:
   *   @[FIPS204, Algorithm 7, line 21] computes r0 = LowBits(w - cs2) and line
   *   23 rejects when ||r0||_inf >= GAMMA2 - BETA. By @[Round3_Spec, Section
   *   5.1] (Lemma 3), this line-23 check on r0 = LowBits(w - cs2) is implied by
   *   ||w0 - cs2||_inf < GAMMA2 - BETA, where w0 is the low part of w. In our
   *   context, w0 already holds the low part of w from the line-13 Decompose;
   *   after subtracting cs2 from it in place, the mld_poly_chknorm(w0, GAMMA2 -
   *   BETA) call below is exactly that check.
   *
   * Hint:
   *   @[FIPS204, Algorithm 7, line 26] sets h = MakeHint(-ct0, w - cs2 + ct0),
   *   and line 28 rejects when ||ct0||_inf >= GAMMA2 or h has more than OMEGA
   *   nonzero coefficients. @[Round3_Spec, Section 5.1] provides the following
   *   alternative description for MakeHint(-ct0, w - cs2 + ct0): a hint bit is
   *   zero exactly when the coefficient of w0 - cs2 + ct0 lies in
   *   (-GAMMA2, GAMMA2], or equals -GAMMA2 while the matching w1 coefficient is
   *   zero (the Decompose border case), and is set otherwise. This equivalence
   *   is precisely what mld_pack_sig_h -> mld_make_hint compute from w0
   *   (= w0 - cs2 + ct0) and w1. The line-28 ||ct0||_inf >= GAMMA2 check is the
   *   mld_poly_chknorm(z, GAMMA2) call on ct0 below; the weight bound is
   *   enforced by mld_pack_sig_h.
   *
   * Building w0 per-component and checking norms incrementally also avoids
   * allocating a full polyveck for h. */
  for (k = 0; k < MLDSA_K; k++)
  __loop__(
    assigns(k,
            object_whole(z),
            object_whole(w0))
    invariant(k <= MLDSA_K)
    invariant(forall(k0, k, MLDSA_K,
      array_abs_bound(w0->vec[k0].coeffs, 0, MLDSA_N, MLDSA_GAMMA2 + 1)))
    decreases(MLDSA_K - k)
  )
  {
    /* @[FIPS204, Algorithm 7, line 19] cs2[k] <- invNTT(c_hat o s2_hat)[k],
     * then subtract from w0[k] to form (w0 - cs2)[k]. */
    mld_sk_s2hat_get_poly(z, s2hat, k);
    mld_poly_pointwise_montgomery(z, cp);
    mld_poly_invntt_tomont(z);

    mld_poly_sub(&w0->vec[k], z);
    mld_poly_reduce(&w0->vec[k]);

    /* Low-bits norm check (see block comment above): the line-23 check on
     * r0 = LowBits(w - cs2) holds via ||w0 - cs2||_inf < GAMMA2 - BETA. */
    w0_invalid = mld_poly_chknorm(&w0->vec[k], MLDSA_GAMMA2 - MLDSA_BETA);
    /* Constant time: w0_invalid may be leaked - see comment for z_invalid. */
    MLD_CT_TESTING_DECLASSIFY(&w0_invalid, sizeof(uint32_t));
    if (w0_invalid)
    {
      ret = MLD_ERR_FAIL; /* reject */
      goto cleanup;
    }

    /* @[FIPS204, Algorithm 7, line 25] ct0[k] <- invNTT(c_hat o t0_hat)[k]. */
    mld_sk_t0hat_get_poly(z, t0hat, k);
    mld_poly_pointwise_montgomery(z, cp);
    mld_poly_invntt_tomont(z);
    mld_poly_reduce(z);

    /* @[FIPS204, Algorithm 7, line 28] reject when ||ct0||_inf >= GAMMA2 (the
     * second part, the OMEGA weight bound, is enforced by mld_pack_sig_h). */
    h_invalid = mld_poly_chknorm(z, MLDSA_GAMMA2);
    /* Constant time: h_invalid may be leaked - see comment for z_invalid. */
    MLD_CT_TESTING_DECLASSIFY(&h_invalid, sizeof(uint32_t));
    if (h_invalid)
    {
      ret = MLD_ERR_FAIL; /* reject */
      goto cleanup;
    }

    /* Add ct0[k] to (w0 - cs2)[k], leaving (w0 - cs2 + ct0)[k] in w0[k] -- the
     * MakeHint input prepared for mld_pack_sig_h (see block comment above). */
    mld_poly_add(&w0->vec[k], z);
  }

  /* Constant time: At this point all norm checks have passed and we, hence,
   * know that the signature does not leak any secret information.
   * Consequently, any value that can be computed from the signature and public
   * key is considered public.
   * w0 and w1 are public as they can be computed from Az - ct = \alpha w1 + w0.
   * h=c*t0 is public as both c and t0 are considered public.
   * While t0 is not part of the public key, it can be reconstructed from
   * a small number of signatures and need not be regarded as secret
   * (see @[FIPS204, Section 6.1]).
   */
  MLD_CT_TESTING_DECLASSIFY(w0, sizeof(*w0));
  MLD_CT_TESTING_DECLASSIFY(w1, sizeof(*w1));

  /* @[FIPS204, Algorithm 7, line 33] sigEncode(ctilde, z mod+/- q, h) is split
   * across three calls: z was already packed by mld_compute_pack_z, this call
   * packs ctilde, and mld_pack_sig_h below packs the hint h. */
  mld_pack_sig_c(sig, challenge_bytes);

  /* @[FIPS204, Algorithm 7, line 26] h <- MakeHint(-ct0, w - cs2 + ct0),
   * computed from (w0 = w0 - cs2 + ct0, w1) as described in the block comment
   * above, and packed as the h component of the line-33 sigEncode. Returns
   * MLD_ERR_FAIL if h would exceed OMEGA nonzero coefficients (the remaining
   * part of the line-28 check), in which case we reject. */
  ret = mld_pack_sig_h(sig, w0, w1);
  if (ret != 0)
  {
    goto cleanup;
  }

  /* Constant time: At this point it is clear that the signature is valid - it
   * can, hence, be considered public. */
  MLD_CT_TESTING_DECLASSIFY(sig, MLDSA_CRYPTO_BYTES);
  ret = 0; /* success */

cleanup:
  /* @[FIPS204, Section 3.6.3] Destruction of intermediate values. */
  MLD_FREE(t, mld_poly, 1, context);
  MLD_FREE(cp, mld_poly, 1, context);
  MLD_FREE(w0, mld_polyveck, 1, context);
  MLD_FREE(w1tmp, w1tmp_u, 1, context);
  MLD_FREE(z, mld_poly, 1, context);
  MLD_FREE(y, mld_yvec, 1, context);
  MLD_FREE(challenge_bytes, uint8_t, MLDSA_CTILDEBYTES, context);

  return ret;
}
MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
int mld_sign_signature_internal(uint8_t sig[MLDSA_CRYPTO_BYTES],
                                const uint8_t *m, size_t mlen,
                                const uint8_t *pre, size_t prelen,
                                const uint8_t rnd[MLDSA_RNDBYTES],
                                const uint8_t sk[MLDSA_CRYPTO_SECRETKEYBYTES],
                                int externalmu,
                                MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
{
  int ret;
  uint8_t *rho, *tr, *key, *mu, *rhoprime;
  uint16_t attempt;
  const uint16_t max_signing_attempts = mld_get_max_signing_attempts();
  MLD_ALLOC(seedbuf, uint8_t,
            2 * MLDSA_SEEDBYTES + MLDSA_TRBYTES + 2 * MLDSA_CRHBYTES, context);
  MLD_ALLOC(mat, mld_polymat, 1, context);
  MLD_ALLOC(s1hat, mld_sk_s1hat, 1, context);
  MLD_ALLOC(t0hat, mld_sk_t0hat, 1, context);
  MLD_ALLOC(s2hat, mld_sk_s2hat, 1, context);

  if (seedbuf == NULL || mat == NULL || s1hat == NULL || t0hat == NULL ||
      s2hat == NULL)
  {
    ret = MLD_ERR_OUT_OF_MEMORY;
    goto cleanup;
  }

  /* If a resume hook is configured via MLD_CONFIG_SIGN_HOOK_RESUME, it provides
   * the attempt to resume from after an earlier pause. Otherwise, we start at
   * 0. Clamp to max_signing_attempts. */
  attempt = mld_sign_resume(context);
  if (attempt > max_signing_attempts)
  {
    attempt = max_signing_attempts;
  }

  rho = seedbuf;
  tr = rho + MLDSA_SEEDBYTES;
  key = tr + MLDSA_TRBYTES;
  mu = key + MLDSA_SEEDBYTES;
  rhoprime = mu + MLDSA_CRHBYTES;
  /* @[FIPS204, Algorithm 7, line 1] (rho, K, tr, s1, s2, t0) <- skDecode(sk)
   * and @[FIPS204, Algorithm 7, lines 2-4] s1_hat/s2_hat/t0_hat <- NTT(...):
   * mld_unpack_sk returns s1hat, s2hat, t0hat already in NTT domain. The spec's
   * private random seed K is held in the local variable key. */
  mld_unpack_sk(rho, tr, key, t0hat, s1hat, s2hat, sk);

  if (!externalmu)
  {
    /* @[FIPS204, Algorithm 7, line 6] mu <- H(BytesToBits(tr) || M', 64). */
    mld_H(mu, MLDSA_CRHBYTES, tr, MLDSA_TRBYTES, pre, prelen, m, mlen);
  }
  else
  {
    /* mu has been provided directly (external-mu variant; line 6 done by the
     * caller in a separate cryptographic module). */
    mld_memcpy(mu, m, MLDSA_CRHBYTES);
  }

  /* @[FIPS204, Algorithm 7, line 7] rhoprime <- H(K || rnd || mu, 64). */
  mld_H(rhoprime, MLDSA_CRHBYTES, key, MLDSA_SEEDBYTES, rnd, MLDSA_RNDBYTES, mu,
        MLDSA_CRHBYTES);

  /* Constant time: rho is part of the public key and, hence, public. */
  MLD_CT_TESTING_DECLASSIFY(rho, MLDSA_SEEDBYTES);
  /* @[FIPS204, Algorithm 7, line 5] A_hat <- ExpandA(rho). */
  mld_polyvec_matrix_expand(mat, rho);

  /* @[FIPS204, Algorithm 7, lines 8-10 and 31-32] the rejection-sampling loop,
   * tracked by attempt (kappa = attempt*MLDSA_L). Each iteration's body (lines
   * 11-30) plus, on success, the line-33 sigEncode are performed by
   * mld_attempt_signature_generation. */

  /* Reference: the reference loops unboundedly; we instead iterate over the
   * bounded range [0, max_signing_attempts) for predictable termination.
   * A success or fatal error exits via goto cleanup; running to completion
   * means every attempt was rejected; with a FIPS compliant choice of
   * MLD_CONFIG_MAX_SIGNING_ATTEMPTS, this should never happen. */
  for (; attempt < max_signing_attempts; attempt++)
  __loop__(
    MLD_IF_NOT_REDUCE_RAM(
      assigns(attempt, ret, memory_slice(sig, MLDSA_CRYPTO_BYTES))
    )
    MLD_IF_REDUCE_RAM(
      assigns(attempt, ret, memory_slice(sig, MLDSA_CRYPTO_BYTES),
              memory_slice(mat, sizeof(mld_polymat)))
    )
    invariant(attempt <= max_signing_attempts)

    /* t0, s1, s2, and mat are initialized above and are NOT changed by this */
    /* loop. We can therefore re-assert their bounds here as part of the     */
    /* loop invariant. This makes proof noticeably faster with CBMC          */
    MLD_IF_NOT_REDUCE_RAM(
      invariant(forall(k1, 0, MLDSA_K, forall(l1, 0, MLDSA_L,
                array_bound(mat->vec[k1].vec[l1].coeffs, 0, MLDSA_N, 0, MLDSA_Q))))
      invariant(forall(k2, 0, MLDSA_K, array_abs_bound(t0hat->vec.vec[k2].coeffs, 0, MLDSA_N, MLD_NTT_BOUND)))
      invariant(forall(k3, 0, MLDSA_L, array_abs_bound(s1hat->vec.vec[k3].coeffs, 0, MLDSA_N, MLD_NTT_BOUND)))
      invariant(forall(k4, 0, MLDSA_K, array_abs_bound(s2hat->vec.vec[k4].coeffs, 0, MLDSA_N, MLD_NTT_BOUND)))
    )
    decreases(max_signing_attempts - attempt)
  )
  {
    /* Safety: attempt < max_signing_attempts <= MLD_MAX_KAPPA / MLDSA_L, so
     * kappa <= MLD_MAX_KAPPA and the cast is safe. */
    const uint16_t kappa = (uint16_t)(attempt * MLDSA_L);

    /* Query configurable signing hook whether signing should be paused.
     * This is skipped by default and only used if the user sets the
     * configuration option MLD_CONFIG_SIGN_HOOK_ATTEMPT. */
    if (mld_sign_attempt(attempt, context) != 0)
    {
      ret = MLD_ERR_SIGNING_PAUSED;
      goto cleanup;
    }

    ret = mld_attempt_signature_generation(sig, mu, rhoprime, kappa, mat, s1hat,
                                           s2hat, t0hat, context);

    /* Decide whether to keep trying based on the return value:
     *  - ret == 0: a valid signature was produced; we are done.
     *  - ret == MLD_ERR_FAIL: this candidate was rejected by one of the norm
     *    or hint checks. We continue the loop and try again with the next
     *    nonce.
     *  - any other value (e.g. MLD_ERR_OUT_OF_MEMORY): an unrecoverable error
     *    occurred, so we propagate it to the caller. */
    if (ret == 0)
    {
      /* Signing succeeded: record the attempt that succeeded. No-op in the
       * default build. */
      mld_sign_finish(attempt, context);
      goto cleanup;
    }
    if (ret != MLD_ERR_FAIL)
    {
      goto cleanup;
    }
  }

  /* Loop ran to completion: all attempts rejected, budget exhausted.
   * This should never happen with a FIPS compliant choice of
   * MLD_CONFIG_MAX_SIGNING_ATTEMPTS. */
  ret = MLD_ERR_SIGN_ATTEMPTS_EXHAUSTED;

cleanup:

  if (ret != 0)
  {
    /* To be on the safe-side, we zeroize the signature buffer. */
    mld_zeroize(sig, MLDSA_CRYPTO_BYTES);
  }

  /* @[FIPS204, Section 3.6.3] Destruction of intermediate values. */
  MLD_FREE(s2hat, mld_sk_s2hat, 1, context);
  MLD_FREE(t0hat, mld_sk_t0hat, 1, context);
  MLD_FREE(s1hat, mld_sk_s1hat, 1, context);
  MLD_FREE(mat, mld_polymat, 1, context);
  MLD_FREE(seedbuf, uint8_t,
           2 * MLDSA_SEEDBYTES + MLDSA_TRBYTES + 2 * MLDSA_CRHBYTES, context);
  return ret;
}

#if !defined(MLD_CONFIG_CORE_API_ONLY)
#if !defined(MLD_CONFIG_NO_RANDOMIZED_API)
MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
int mld_sign_signature(uint8_t sig[MLDSA_CRYPTO_BYTES], const uint8_t *m,
                       size_t mlen, const uint8_t *ctx, size_t ctxlen,
                       const uint8_t sk[MLDSA_CRYPTO_SECRETKEYBYTES],
                       MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
{
  size_t pre_len;
  int ret;
  MLD_ALLOC(pre, uint8_t, MLD_DOMAIN_SEPARATION_MAX_BYTES, context);
  MLD_ALLOC(rnd, uint8_t, MLDSA_RNDBYTES, context);

  if (pre == NULL || rnd == NULL)
  {
    ret = MLD_ERR_OUT_OF_MEMORY;
    goto cleanup;
  }

  /* Prepare domain separation prefix for pure ML-DSA */
  pre_len = mld_prepare_domain_separation_prefix(pre, NULL, 0, ctx, ctxlen,
                                                 MLD_PREHASH_NONE);
  if (pre_len == 0)
  {
    ret = MLD_ERR_INVALID_ARG;
    goto cleanup;
  }

  /* Randomized variant of ML-DSA. If you need the deterministic variant,
   * call mld_sign_signature_internal directly with all-zero rnd. */
  if (mld_randombytes(rnd, MLDSA_RNDBYTES) != 0)
  {
    ret = MLD_ERR_RNG_FAIL;
    goto cleanup;
  }
  MLD_CT_TESTING_SECRET(rnd, MLDSA_RNDBYTES);

  ret = mld_sign_signature_internal(sig, m, mlen, pre, pre_len, rnd, sk, 0,
                                    context);

cleanup:
  if (ret != 0)
  {
    /* To be on the safe-side, make sure sig has a well-defined value, even in
     * the case of error.
     *
     * If we come from mld_sign_signature_internal, this is redundant, but the
     * error case should not be the norm, and the added cost of the zeroization
     * insignificant. */
    mld_zeroize(sig, MLDSA_CRYPTO_BYTES);
  }

  /* @[FIPS204, Section 3.6.3] Destruction of intermediate values. */
  MLD_FREE(rnd, uint8_t, MLDSA_RNDBYTES, context);
  MLD_FREE(pre, uint8_t, MLD_DOMAIN_SEPARATION_MAX_BYTES, context);

  return ret;
}
#endif /* !MLD_CONFIG_NO_RANDOMIZED_API */

#if !defined(MLD_CONFIG_NO_RANDOMIZED_API)
MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
int mld_sign_signature_extmu(uint8_t sig[MLDSA_CRYPTO_BYTES],
                             const uint8_t mu[MLDSA_CRHBYTES],
                             const uint8_t sk[MLDSA_CRYPTO_SECRETKEYBYTES],
                             MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
{
  int ret;
  MLD_ALLOC(rnd, uint8_t, MLDSA_RNDBYTES, context);

  if (rnd == NULL)
  {
    ret = MLD_ERR_OUT_OF_MEMORY;
    goto cleanup;
  }

  /* Randomized variant of ML-DSA. If you need the deterministic variant,
   * call mld_sign_signature_internal directly with all-zero rnd. */
  if (mld_randombytes(rnd, MLDSA_RNDBYTES) != 0)
  {
    ret = MLD_ERR_RNG_FAIL;
    goto cleanup;
  }
  MLD_CT_TESTING_SECRET(rnd, MLDSA_RNDBYTES);

  ret = mld_sign_signature_internal(sig, mu, MLDSA_CRHBYTES, NULL, 0, rnd, sk,
                                    1, context);

cleanup:
  /* @[FIPS204, Section 3.6.3] Destruction of intermediate values. */
  MLD_FREE(rnd, uint8_t, MLDSA_RNDBYTES, context);

  return ret;
}
#endif /* !MLD_CONFIG_NO_RANDOMIZED_API */
#endif /* !MLD_CONFIG_CORE_API_ONLY */
#endif /* !MLD_CONFIG_NO_SIGN_API */

#if !defined(MLD_CONFIG_NO_VERIFY_API)
MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
int mld_sign_verify_internal(const uint8_t sig[MLDSA_CRYPTO_BYTES],
                             const uint8_t *m, size_t mlen, const uint8_t *pre,
                             size_t prelen,
                             const uint8_t pk[MLDSA_CRYPTO_PUBLICKEYBYTES],
                             int externalmu,
                             MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
{
  int ret, cmp;
  unsigned int i;

  MLD_ALLOC(buf, uint8_t, (MLDSA_K * MLDSA_POLYW1_PACKEDBYTES), context);
  MLD_ALLOC(mu, uint8_t, MLDSA_CRHBYTES, context);
  MLD_ALLOC(c, uint8_t, MLDSA_CTILDEBYTES, context);
  MLD_ALLOC(c2, uint8_t, MLDSA_CTILDEBYTES, context);
  MLD_ALLOC(z, mld_polyvecl, 1, context);
  MLD_ALLOC(cp, mld_poly, 1, context);
  MLD_ALLOC(mat, mld_polymat, 1, context);
  MLD_ALLOC(w1, mld_poly, 1, context);
  MLD_ALLOC(tmp, mld_poly, 1, context);

  if (buf == NULL || mu == NULL || c == NULL || c2 == NULL || z == NULL ||
      cp == NULL || mat == NULL || w1 == NULL || tmp == NULL)
  {
    ret = MLD_ERR_OUT_OF_MEMORY;
    goto cleanup;
  }

  mld_memcpy(c, sig, MLDSA_CTILDEBYTES);
  mld_polyvecl_unpack_z(z, sig + MLDSA_SIG_Z_OFFSET);

  /* mld_polyvecl_chknorm signals failure through a single non-zero error code
   * that's not yet aligned with MLD_ERR_XXX. A norm-check failure here means
   * the signature is invalid, so map it to MLD_ERR_INVALID_SIGNATURE. */
  if (mld_polyvecl_chknorm(z, MLDSA_GAMMA1 - MLDSA_BETA))
  {
    ret = MLD_ERR_INVALID_SIGNATURE;
    goto cleanup;
  }

  if (!externalmu)
  {
    /* Compute CRH(H(rho, t1), pre, msg) */
    MLD_ALIGN uint8_t hpk[MLDSA_CRHBYTES];
    mld_H(hpk, MLDSA_TRBYTES, pk, MLDSA_CRYPTO_PUBLICKEYBYTES, NULL, 0, NULL,
          0);
    mld_H(mu, MLDSA_CRHBYTES, hpk, MLDSA_TRBYTES, pre, prelen, m, mlen);

    /* @[FIPS204, Section 3.6.3] Destruction of intermediate values. */
    mld_zeroize(hpk, sizeof(hpk));
  }
  else
  {
    /* mu has been provided directly */
    mld_memcpy(mu, m, MLDSA_CRHBYTES);
  }

  /* Matrix-vector multiplication and per-row reconstruction of w1. */
  mld_polyvecl_ntt(z);
  mld_polyvec_matrix_expand(mat, pk);
  mld_poly_challenge(cp, c);
  mld_poly_ntt(cp);

  for (i = 0; i < MLDSA_K; ++i)
  __loop__(
    assigns(MLD_IF_REDUCE_RAM(memory_slice(mat, sizeof(mld_polymat)),)
            i, ret,
            memory_slice(w1, sizeof(mld_poly)),
            memory_slice(tmp, sizeof(mld_poly)),
            memory_slice(buf, MLDSA_K * MLDSA_POLYW1_PACKEDBYTES)
    )
    invariant(i <= MLDSA_K)
    decreases(MLDSA_K - i)
  )
  {
    /* w1 = (A * z)_i in NTT domain */
    mld_polyvec_matrix_pointwise_montgomery_row(w1, mat, z, i);

    /* tmp = c * t1_i * 2^d in NTT domain */
    mld_unpack_pk_t1(tmp, pk, i);
    mld_poly_shiftl(tmp);
    mld_poly_ntt(tmp);
    mld_poly_pointwise_montgomery(tmp, cp);

    /* w1 = invNTT(w1 - c * t1_i * 2^d) */
    mld_poly_sub(w1, tmp);
    mld_poly_reduce(w1);
    mld_poly_invntt_tomont(w1);
    mld_poly_caddq(w1);

    /* tmp = h_i (decoded and validated from signature). A non-zero return
     * means the hint encoding is malformed, i.e. the signature is invalid. */
    if (mld_sig_unpack_hints(tmp, sig, i) != 0)
    {
      ret = MLD_ERR_INVALID_SIGNATURE;
      goto cleanup;
    }

    /* w1 = use_hint(w1, tmp), then pack into buf[i] */
    mld_poly_use_hint(w1, tmp);
    mld_polyw1_pack(buf + i * MLDSA_POLYW1_PACKEDBYTES, w1);
  }

  /* Call random oracle and verify challenge */
  mld_H(c2, MLDSA_CTILDEBYTES, mu, MLDSA_CRHBYTES, buf,
        MLDSA_K * MLDSA_POLYW1_PACKEDBYTES, NULL, 0);

  cmp = mld_ct_memcmp(c, c2, MLDSA_CTILDEBYTES);

  /* Declassify the result of the verification. */
  MLD_CT_TESTING_DECLASSIFY(&cmp, sizeof(cmp));

  ret = cmp == 0 ? 0 : MLD_ERR_INVALID_SIGNATURE;

cleanup:
  /* @[FIPS204, Section 3.6.3] Destruction of intermediate values. */
  MLD_FREE(tmp, mld_poly, 1, context);
  MLD_FREE(w1, mld_poly, 1, context);
  MLD_FREE(mat, mld_polymat, 1, context);
  MLD_FREE(cp, mld_poly, 1, context);
  MLD_FREE(z, mld_polyvecl, 1, context);
  MLD_FREE(c2, uint8_t, MLDSA_CTILDEBYTES, context);
  MLD_FREE(c, uint8_t, MLDSA_CTILDEBYTES, context);
  MLD_FREE(mu, uint8_t, MLDSA_CRHBYTES, context);
  MLD_FREE(buf, uint8_t, (MLDSA_K * MLDSA_POLYW1_PACKEDBYTES), context);
  return ret;
}

#if !defined(MLD_CONFIG_CORE_API_ONLY)
MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
int mld_sign_verify(const uint8_t sig[MLDSA_CRYPTO_BYTES], const uint8_t *m,
                    size_t mlen, const uint8_t *ctx, size_t ctxlen,
                    const uint8_t pk[MLDSA_CRYPTO_PUBLICKEYBYTES],
                    MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
{
  MLD_ALIGN uint8_t pre[MLD_DOMAIN_SEPARATION_MAX_BYTES];
  size_t pre_len;
  int ret;

  pre_len = mld_prepare_domain_separation_prefix(pre, NULL, 0, ctx, ctxlen,
                                                 MLD_PREHASH_NONE);
  if (pre_len == 0)
  {
    ret = MLD_ERR_INVALID_ARG;
    goto cleanup;
  }

  ret = mld_sign_verify_internal(sig, m, mlen, pre, pre_len, pk, 0, context);

cleanup:
  /* @[FIPS204, Section 3.6.3] Destruction of intermediate values. */
  mld_zeroize(pre, sizeof(pre));

  return ret;
}

MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
int mld_sign_verify_extmu(const uint8_t sig[MLDSA_CRYPTO_BYTES],
                          const uint8_t mu[MLDSA_CRHBYTES],
                          const uint8_t pk[MLDSA_CRYPTO_PUBLICKEYBYTES],
                          MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
{
  return mld_sign_verify_internal(sig, mu, MLDSA_CRHBYTES, NULL, 0, pk, 1,
                                  context);
}
#endif /* !MLD_CONFIG_CORE_API_ONLY */
#endif /* !MLD_CONFIG_NO_VERIFY_API */

#if !defined(MLD_CONFIG_CORE_API_ONLY)
#if !defined(MLD_CONFIG_NO_SIGN_API)
MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
int mld_sign_signature_pre_hash_internal(
    uint8_t sig[MLDSA_CRYPTO_BYTES], const uint8_t *ph, size_t phlen,
    const uint8_t *ctx, size_t ctxlen, const uint8_t rnd[MLDSA_RNDBYTES],
    const uint8_t sk[MLDSA_CRYPTO_SECRETKEYBYTES], int hashalg,
    MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
{
  MLD_ALIGN uint8_t pre[MLD_DOMAIN_SEPARATION_MAX_BYTES];
  size_t pre_len;
  int ret;

  if (hashalg == MLD_PREHASH_NONE)
  {
    ret = MLD_ERR_INVALID_ARG;
    goto cleanup;
  }

  pre_len = mld_prepare_domain_separation_prefix(pre, ph, phlen, ctx, ctxlen,
                                                 hashalg);
  if (pre_len == 0)
  {
    ret = MLD_ERR_INVALID_ARG;
    goto cleanup;
  }

  ret = mld_sign_signature_internal(sig, pre, pre_len, NULL, 0, rnd, sk, 0,
                                    context);
cleanup:
  if (ret != 0)
  {
    /* To be on the safe-side, make sure sig has a well-defined value, even in
     * the case of error.
     *
     * If we come from mld_sign_signature_internal, this is redundant, but the
     * error case should not be the norm, and the added cost of the zeroization
     * insignificant. */
    mld_zeroize(sig, MLDSA_CRYPTO_BYTES);
  }

  /* @[FIPS204, Section 3.6.3] Destruction of intermediate values. */
  mld_zeroize(pre, sizeof(pre));
  return ret;
}
#endif /* !MLD_CONFIG_NO_SIGN_API */

#if !defined(MLD_CONFIG_NO_VERIFY_API)
MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
int mld_sign_verify_pre_hash_internal(
    const uint8_t sig[MLDSA_CRYPTO_BYTES], const uint8_t *ph, size_t phlen,
    const uint8_t *ctx, size_t ctxlen,
    const uint8_t pk[MLDSA_CRYPTO_PUBLICKEYBYTES], int hashalg,
    MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
{
  MLD_ALIGN uint8_t pre[MLD_DOMAIN_SEPARATION_MAX_BYTES];
  size_t pre_len;
  int ret;

  if (hashalg == MLD_PREHASH_NONE)
  {
    ret = MLD_ERR_INVALID_ARG;
    goto cleanup;
  }

  pre_len = mld_prepare_domain_separation_prefix(pre, ph, phlen, ctx, ctxlen,
                                                 hashalg);
  if (pre_len == 0)
  {
    ret = MLD_ERR_INVALID_ARG;
    goto cleanup;
  }

  ret = mld_sign_verify_internal(sig, pre, pre_len, NULL, 0, pk, 0, context);

cleanup:
  /* @[FIPS204, Section 3.6.3] Destruction of intermediate values. */
  mld_zeroize(pre, sizeof(pre));
  return ret;
}
#endif /* !MLD_CONFIG_NO_VERIFY_API */

#if !defined(MLD_CONFIG_NO_SIGN_API)
MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
int mld_sign_signature_pre_hash_shake256(
    uint8_t sig[MLDSA_CRYPTO_BYTES], const uint8_t *m, size_t mlen,
    const uint8_t *ctx, size_t ctxlen, const uint8_t rnd[MLDSA_RNDBYTES],
    const uint8_t sk[MLDSA_CRYPTO_SECRETKEYBYTES],
    MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
{
  MLD_ALIGN uint8_t ph[64];
  int ret;
  mld_shake256(ph, sizeof(ph), m, mlen);
  ret = mld_sign_signature_pre_hash_internal(sig, ph, sizeof(ph), ctx, ctxlen,
                                             rnd, sk, MLD_PREHASH_SHAKE_256,
                                             context);
  /* @[FIPS204, Section 3.6.3] Destruction of intermediate values. */
  mld_zeroize(ph, sizeof(ph));
  return ret;
}
#endif /* !MLD_CONFIG_NO_SIGN_API */

#if !defined(MLD_CONFIG_NO_VERIFY_API)
MLD_MUST_CHECK_RETURN_VALUE
MLD_EXTERNAL_API
int mld_sign_verify_pre_hash_shake256(
    const uint8_t sig[MLDSA_CRYPTO_BYTES], const uint8_t *m, size_t mlen,
    const uint8_t *ctx, size_t ctxlen,
    const uint8_t pk[MLDSA_CRYPTO_PUBLICKEYBYTES],
    MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
{
  MLD_ALIGN uint8_t ph[64];
  int ret;
  mld_shake256(ph, sizeof(ph), m, mlen);
  ret = mld_sign_verify_pre_hash_internal(sig, ph, sizeof(ph), ctx, ctxlen, pk,
                                          MLD_PREHASH_SHAKE_256, context);
  /* @[FIPS204, Section 3.6.3] Destruction of intermediate values. */
  mld_zeroize(ph, sizeof(ph));
  return ret;
}
#endif /* !MLD_CONFIG_NO_VERIFY_API */

#if !defined(MLD_CONFIG_NO_SIGN_API) || !defined(MLD_CONFIG_NO_VERIFY_API)
#define MLD_PRE_HASH_OID_LEN 11

/**
 * Return the OID of a given SHA-2/SHA-3 hash function.
 *
 * @param[out] oid     Pointer to output OID.
 * @param      hashalg Hash algorithm constant (MLD_PREHASH_*).
 */
static void mld_get_hash_oid(uint8_t oid[MLD_PRE_HASH_OID_LEN], int hashalg)
{
  unsigned int i;
  static const struct
  {
    int alg;
    uint8_t oid[MLD_PRE_HASH_OID_LEN];
  } oid_map[] = {
      {MLD_PREHASH_SHA2_224,
       {0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x04}},
      {MLD_PREHASH_SHA2_256,
       {0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01}},
      {MLD_PREHASH_SHA2_384,
       {0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02}},
      {MLD_PREHASH_SHA2_512,
       {0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03}},
      {MLD_PREHASH_SHA2_512_224,
       {0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x05}},
      {MLD_PREHASH_SHA2_512_256,
       {0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x06}},
      {MLD_PREHASH_SHA3_224,
       {0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x07}},
      {MLD_PREHASH_SHA3_256,
       {0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x08}},
      {MLD_PREHASH_SHA3_384,
       {0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x09}},
      {MLD_PREHASH_SHA3_512,
       {0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x0A}},
      {MLD_PREHASH_SHAKE_128,
       {0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x0B}},
      {MLD_PREHASH_SHAKE_256,
       {0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x0C}}};

  for (i = 0; i < sizeof(oid_map) / sizeof(oid_map[0]); i++)
  __loop__(
    invariant(i <= sizeof(oid_map) / sizeof(oid_map[0]))
    decreases(sizeof(oid_map) / sizeof(oid_map[0]) - i)
  )
  {
    if (oid_map[i].alg == hashalg)
    {
      mld_memcpy(oid, oid_map[i].oid, MLD_PRE_HASH_OID_LEN);
      return;
    }
  }
}

static int mld_validate_hash_length(int hashalg, size_t len)
{
  switch (hashalg)
  {
    case MLD_PREHASH_SHA2_224:
      return (len == 224 / 8) ? 0 : -1;
    case MLD_PREHASH_SHA2_256:
      return (len == 256 / 8) ? 0 : -1;
    case MLD_PREHASH_SHA2_384:
      return (len == 384 / 8) ? 0 : -1;
    case MLD_PREHASH_SHA2_512:
      return (len == 512 / 8) ? 0 : -1;
    case MLD_PREHASH_SHA2_512_224:
      return (len == 224 / 8) ? 0 : -1;
    case MLD_PREHASH_SHA2_512_256:
      return (len == 256 / 8) ? 0 : -1;
    case MLD_PREHASH_SHA3_224:
      return (len == 224 / 8) ? 0 : -1;
    case MLD_PREHASH_SHA3_256:
      return (len == 256 / 8) ? 0 : -1;
    case MLD_PREHASH_SHA3_384:
      return (len == 384 / 8) ? 0 : -1;
    case MLD_PREHASH_SHA3_512:
      return (len == 512 / 8) ? 0 : -1;
    case MLD_PREHASH_SHAKE_128:
      return (len == 256 / 8) ? 0 : -1;
    case MLD_PREHASH_SHAKE_256:
      return (len == 512 / 8) ? 0 : -1;
    default:
      return -1;
  }
}

MLD_EXTERNAL_API
size_t mld_prepare_domain_separation_prefix(
    uint8_t prefix[MLD_DOMAIN_SEPARATION_MAX_BYTES], const uint8_t *ph,
    size_t phlen, const uint8_t *ctx, size_t ctxlen, int hashalg)
{
  if (ctxlen > 255)
  {
    return 0;
  }

  if (hashalg != MLD_PREHASH_NONE)
  {
    if (ph == NULL || mld_validate_hash_length(hashalg, phlen) != 0)
    {
      return 0;
    }
  }

  /* Common prefix: 0x00/0x01 || ctxlen || ctx */
  prefix[0] = (hashalg == MLD_PREHASH_NONE) ? 0 : 1;
  prefix[1] = (uint8_t)ctxlen;
  if (ctxlen > 0)
  {
    mld_memcpy(prefix + 2, ctx, ctxlen);
  }

  if (hashalg == MLD_PREHASH_NONE)
  {
    return 2 + ctxlen;
  }

  /* HashML-DSA: append oid || ph */
  mld_get_hash_oid(prefix + 2 + ctxlen, hashalg);
  mld_memcpy(prefix + 2 + ctxlen + MLD_PRE_HASH_OID_LEN, ph, phlen);
  return 2 + ctxlen + MLD_PRE_HASH_OID_LEN + phlen;
}
#endif /* !MLD_CONFIG_NO_SIGN_API || !MLD_CONFIG_NO_VERIFY_API */

#if !defined(MLD_CONFIG_NO_KEYPAIR_API)
MLD_EXTERNAL_API
int mld_sign_pk_from_sk(uint8_t pk[MLDSA_CRYPTO_PUBLICKEYBYTES],
                        const uint8_t sk[MLDSA_CRYPTO_SECRETKEYBYTES],
                        MLD_CONFIG_CONTEXT_PARAMETER_TYPE context)
{
  uint8_t check, cmp0, cmp1, chk1, chk2;
  int ret;
  MLD_ALLOC(rho, uint8_t, MLDSA_SEEDBYTES, context);
  MLD_ALLOC(tr, uint8_t, MLDSA_TRBYTES, context);
  MLD_ALLOC(tr_computed, uint8_t, MLDSA_TRBYTES, context);
  MLD_ALLOC(key, uint8_t, MLDSA_SEEDBYTES, context);
  MLD_ALLOC(s1, mld_polyvecl, 1, context);
  MLD_ALLOC(s2, mld_polyveck, 1, context);
  MLD_ALLOC(t0_packed, uint8_t, MLDSA_K *MLDSA_POLYT0_PACKEDBYTES, context);

  if (rho == NULL || tr == NULL || tr_computed == NULL || key == NULL ||
      s1 == NULL || s2 == NULL || t0_packed == NULL)
  {
    ret = MLD_ERR_OUT_OF_MEMORY;
    goto cleanup;
  }

  /* Inline unpack_sk: mld_unpack_sk uses lazy types for s1/s2/t0 which
   * we cannot use here. t0 stays in packed form -- we compare it against
   * the recomputed value below. */
  mld_memcpy(rho, sk + MLDSA_SK_RHO_OFFSET, MLDSA_SEEDBYTES);
  mld_memcpy(key, sk + MLDSA_SK_KEY_OFFSET, MLDSA_SEEDBYTES);
  mld_memcpy(tr, sk + MLDSA_SK_TR_OFFSET, MLDSA_TRBYTES);
  mld_polyvecl_unpack_eta(s1, sk + MLDSA_SK_S1_OFFSET);
  mld_polyveck_unpack_eta(s2, sk + MLDSA_SK_S2_OFFSET);

  /* Validate s1 and s2 coefficients are within [-MLDSA_ETA, MLDSA_ETA] */
  chk1 = mld_polyvecl_chknorm(s1, MLDSA_ETA + 1) & 0xFF;
  chk2 = mld_polyveck_chknorm(s2, MLDSA_ETA + 1) & 0xFF;

  /* NTT s1 in place to use as s1hat */
  mld_polyvecl_ntt(s1);

  /* Pack rho into pk */
  mld_memcpy(pk + MLDSA_PK_RHO_OFFSET, rho, MLDSA_SEEDBYTES);

  /* Recompute t row by row, decompose, and pack t1 into pk and t0 into
   * t0_packed. */
  ret = mld_compute_pack_t0_t1(pk + MLDSA_PK_T1_OFFSET, t0_packed, s1, s2, rho,
                               context);
  if (ret != 0)
  {
    goto cleanup;
  }

  /* Compare recomputed packed t0 against the t0 region of sk. */
  cmp0 = mld_ct_memcmp(t0_packed, sk + MLDSA_SK_T0_OFFSET,
                       MLDSA_K * MLDSA_POLYT0_PACKEDBYTES);

  /* Compute tr_computed = H(pk) and compare to the stored tr */
  mld_shake256(tr_computed, MLDSA_TRBYTES, pk, MLDSA_CRYPTO_PUBLICKEYBYTES);
  cmp1 = mld_ct_memcmp((const uint8_t *)tr, (const uint8_t *)tr_computed,
                       MLDSA_TRBYTES);
  check = mld_value_barrier_u8(cmp0 | cmp1 | chk1 | chk2);

  /* Declassify the final result of the validity check. */
  MLD_CT_TESTING_DECLASSIFY(&check, sizeof(check));
  ret = (check != 0) ? MLD_ERR_INVALID_KEY : 0;

cleanup:

  if (ret != 0)
  {
    mld_zeroize(pk, MLDSA_CRYPTO_PUBLICKEYBYTES);
  }

  /* Constant time: pk is either the valid public key or zeroed on error */
  MLD_CT_TESTING_DECLASSIFY(pk, MLDSA_CRYPTO_PUBLICKEYBYTES);

  /* @[FIPS204, Section 3.6.3] Destruction of intermediate values. */
  MLD_FREE(t0_packed, uint8_t, MLDSA_K *MLDSA_POLYT0_PACKEDBYTES, context);
  MLD_FREE(s2, mld_polyveck, 1, context);
  MLD_FREE(s1, mld_polyvecl, 1, context);
  MLD_FREE(key, uint8_t, MLDSA_SEEDBYTES, context);
  MLD_FREE(tr_computed, uint8_t, MLDSA_TRBYTES, context);
  MLD_FREE(tr, uint8_t, MLDSA_TRBYTES, context);
  MLD_FREE(rho, uint8_t, MLDSA_SEEDBYTES, context);

  return ret;
}
#endif /* !MLD_CONFIG_NO_KEYPAIR_API */
#endif /* !MLD_CONFIG_CORE_API_ONLY */

/* To facilitate single-compilation-unit (SCU) builds, undefine all macros.
 * Don't modify by hand -- this is auto-generated by scripts/autogen. */
#undef mld_check_pct
#undef mld_sample_s1_s2
#undef mld_validate_hash_length
#undef mld_get_hash_oid
#undef mld_H
#undef mld_compute_pack_z
#undef mld_attempt_signature_generation
#undef mld_compute_pack_t0_t1
#undef mld_get_max_signing_attempts
#undef MLD_MAX_SIGNING_ATTEMPTS
#undef MLD_PRE_HASH_OID_LEN
