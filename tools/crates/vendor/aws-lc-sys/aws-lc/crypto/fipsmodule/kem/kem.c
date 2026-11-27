// Copyright Amazon.com Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

#include <openssl/base.h>

#include "../delocate.h"
#include "../ml_kem/ml_kem.h"
#include "internal.h"
#include <openssl/bytestring.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/evp_errors.h>
#include <openssl/mem.h>

#include "../cpucap/internal.h"
#include "../service_indicator/internal.h"

// https://csrc.nist.gov/projects/computer-security-objects-register/algorithm-registration
// 2.16.840.1.101.3.4.4.1
static const uint8_t kOIDMLKEM512[]  = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x04, 0x01};
// 2.16.840.1.101.3.4.4.2
static const uint8_t kOIDMLKEM768[]  = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x04, 0x02};
// 2.16.840.1.101.3.4.4.3
static const uint8_t kOIDMLKEM1024[] = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x04, 0x03};

static int ml_kem_1024_keygen_deterministic(uint8_t *public_key,
                                            size_t *public_len,
                                            uint8_t *secret_key,
                                            size_t *secret_len,
                                            const uint8_t *seed) {
  return ml_kem_1024_keypair_deterministic(public_key, public_len, secret_key, secret_len, seed) == 0;
}

static int ml_kem_1024_encaps_deterministic(uint8_t *ciphertext,
                                            size_t *ciphertext_len,
                                            uint8_t *shared_secret,
                                            size_t *shared_secret_len,
                                            const uint8_t *public_key,
                                            const uint8_t *seed) {
  return ml_kem_1024_encapsulate_deterministic(ciphertext, ciphertext_len, shared_secret, shared_secret_len, public_key, seed) == 0;
}

static int ml_kem_1024_encaps(uint8_t *ciphertext,
                              size_t *ciphertext_len,
                              uint8_t *shared_secret,
                              size_t *shared_secret_len,
                              const uint8_t *public_key) {
  return ml_kem_1024_encapsulate(ciphertext, ciphertext_len, shared_secret, shared_secret_len, public_key) == 0;
}

static int ml_kem_1024_decaps(uint8_t *shared_secret,
                              size_t *shared_secret_len,
                              const uint8_t *ciphertext,
                              const uint8_t *secret_key) {
  return ml_kem_1024_decapsulate(shared_secret, shared_secret_len, ciphertext, secret_key) == 0;
}

DEFINE_LOCAL_DATA(KEM_METHOD, kem_ml_kem_1024_method) {
  out->keygen_deterministic = ml_kem_1024_keygen_deterministic;
  out->encaps_deterministic = ml_kem_1024_encaps_deterministic;
  out->encaps = ml_kem_1024_encaps;
  out->decaps = ml_kem_1024_decaps;
}

static int ml_kem_768_keygen_deterministic(uint8_t *public_key,
                                           size_t *public_len,
                                           uint8_t *secret_key,
                                           size_t *secret_len,
                                           const uint8_t *seed) {
  return ml_kem_768_keypair_deterministic(public_key, public_len, secret_key, secret_len, seed) == 0;
}

static int ml_kem_768_encaps_deterministic(uint8_t *ciphertext,
                                           size_t *ciphertext_len,
                                           uint8_t *shared_secret,
                                           size_t *shared_secret_len,
                                           const uint8_t *public_key,
                                           const uint8_t *seed) {
  return ml_kem_768_encapsulate_deterministic(ciphertext, ciphertext_len, shared_secret, shared_secret_len, public_key, seed) == 0;
}

static int ml_kem_768_encaps(uint8_t *ciphertext,
                             size_t *ciphertext_len,
                             uint8_t *shared_secret,
                             size_t *shared_secret_len,
                             const uint8_t *public_key) {
  return ml_kem_768_encapsulate(ciphertext, ciphertext_len, shared_secret, shared_secret_len, public_key) == 0;
}

static int ml_kem_768_decaps(uint8_t *shared_secret,
                             size_t *shared_secret_len,
                             const uint8_t *ciphertext,
                             const uint8_t *secret_key) {
  return ml_kem_768_decapsulate(shared_secret, shared_secret_len, ciphertext, secret_key) == 0;
}

DEFINE_LOCAL_DATA(KEM_METHOD, kem_ml_kem_768_method) {
  out->keygen_deterministic = ml_kem_768_keygen_deterministic;
  out->encaps_deterministic = ml_kem_768_encaps_deterministic;
  out->encaps = ml_kem_768_encaps;
  out->decaps = ml_kem_768_decaps;
}

static int ml_kem_512_keygen_deterministic(uint8_t *public_key,
                                           size_t *public_len,
                                           uint8_t *secret_key,
                                           size_t *secret_len,
                                           const uint8_t *seed) {
  return ml_kem_512_keypair_deterministic(public_key, public_len, secret_key, secret_len, seed) == 0;
}

static int ml_kem_512_encaps_deterministic(uint8_t *ciphertext,
                                           size_t *ciphertext_len,
                                           uint8_t *shared_secret,
                                           size_t *shared_secret_len,
                                           const uint8_t *public_key,
                                           const uint8_t *seed) {
  return ml_kem_512_encapsulate_deterministic(ciphertext, ciphertext_len, shared_secret, shared_secret_len, public_key, seed) == 0;
}

static int ml_kem_512_encaps(uint8_t *ciphertext,
                             size_t *ciphertext_len,
                             uint8_t *shared_secret,
                             size_t *shared_secret_len,
                             const uint8_t *public_key) {
  return ml_kem_512_encapsulate(ciphertext, ciphertext_len, shared_secret, shared_secret_len, public_key) == 0;
}

static int ml_kem_512_decaps(uint8_t *shared_secret,
                             size_t *shared_secret_len,
                             const uint8_t *ciphertext,
                             const uint8_t *secret_key) {
  return ml_kem_512_decapsulate(shared_secret, shared_secret_len, ciphertext, secret_key) == 0;
}

DEFINE_LOCAL_DATA(KEM_METHOD, kem_ml_kem_512_method) {
  out->keygen_deterministic = ml_kem_512_keygen_deterministic;
  out->encaps_deterministic = ml_kem_512_encaps_deterministic;
  out->encaps = ml_kem_512_encaps;
  out->decaps = ml_kem_512_decaps;
}

DEFINE_LOCAL_DATA(KEM, KEM_ml_kem_512) {
  out->nid = NID_MLKEM512;
  out->oid = kOIDMLKEM512;
  out->oid_len = sizeof(kOIDMLKEM512);
  out->comment = "MLKEM512 ";
  out->public_key_len = MLKEM512_PUBLIC_KEY_BYTES;
  out->secret_key_len = MLKEM512_SECRET_KEY_BYTES;
  out->ciphertext_len = MLKEM512_CIPHERTEXT_BYTES;
  out->shared_secret_len = MLKEM512_SHARED_SECRET_LEN;
  out->keygen_seed_len = MLKEM512_KEYGEN_SEED_LEN;
  out->encaps_seed_len = MLKEM512_ENCAPS_SEED_LEN;
  out->method = kem_ml_kem_512_method();
}

DEFINE_LOCAL_DATA(KEM, KEM_ml_kem_768) {
  out->nid = NID_MLKEM768;
  out->oid = kOIDMLKEM768;
  out->oid_len = sizeof(kOIDMLKEM768);
  out->comment = "MLKEM768 ";
  out->public_key_len = MLKEM768_PUBLIC_KEY_BYTES;
  out->secret_key_len = MLKEM768_SECRET_KEY_BYTES;
  out->ciphertext_len = MLKEM768_CIPHERTEXT_BYTES;
  out->shared_secret_len = MLKEM768_SHARED_SECRET_LEN;
  out->keygen_seed_len = MLKEM768_KEYGEN_SEED_LEN;
  out->encaps_seed_len = MLKEM768_ENCAPS_SEED_LEN;
  out->method = kem_ml_kem_768_method();
}

DEFINE_LOCAL_DATA(KEM, KEM_ml_kem_1024) {
  out->nid = NID_MLKEM1024;
  out->oid = kOIDMLKEM1024;
  out->oid_len = sizeof(kOIDMLKEM1024);
  out->comment = "MLKEM1024 ";
  out->public_key_len = MLKEM1024_PUBLIC_KEY_BYTES;
  out->secret_key_len = MLKEM1024_SECRET_KEY_BYTES;
  out->ciphertext_len = MLKEM1024_CIPHERTEXT_BYTES;
  out->shared_secret_len = MLKEM1024_SHARED_SECRET_LEN;
  out->keygen_seed_len = MLKEM1024_KEYGEN_SEED_LEN;
  out->encaps_seed_len = MLKEM1024_ENCAPS_SEED_LEN;
  out->method = kem_ml_kem_1024_method();
}

const KEM *KEM_find_kem_by_nid(int nid) {
  switch (nid) {
    case NID_MLKEM512:
      return KEM_ml_kem_512();
    case NID_MLKEM768:
      return KEM_ml_kem_768();
    case NID_MLKEM1024:
      return KEM_ml_kem_1024();
    default:
      return NULL;
  }
}

KEM_KEY *KEM_KEY_new(void) {
  KEM_KEY *ret = OPENSSL_zalloc(sizeof(KEM_KEY));
  if (ret == NULL) {
    return NULL;
  }

  return ret;
}

static void KEM_KEY_clear(KEM_KEY *key) {
  key->kem = NULL;
  OPENSSL_free(key->public_key);
  OPENSSL_free(key->secret_key);
  OPENSSL_free(key->seed);
  key->public_key = NULL;
  key->secret_key = NULL;
  key->seed = NULL;
}

int KEM_KEY_init(KEM_KEY *key, const KEM *kem) {
  if (key == NULL || kem == NULL) {
    return 0;
  }
  // If the key is already initialized clear it.
  KEM_KEY_clear(key);

  key->kem = kem;
  key->public_key = OPENSSL_malloc(kem->public_key_len);
  key->secret_key = OPENSSL_malloc(kem->secret_key_len);
  if (key->public_key == NULL || key->secret_key == NULL) {
    KEM_KEY_clear(key);
    return 0;
  }

  return 1;
}

const EVP_PKEY_ASN1_METHOD *KEM_find_asn1_by_nid(int nid) {
  switch (nid) {
    case NID_MLKEM512:
    case NID_MLKEM768:
    case NID_MLKEM1024:
      return &kem_asn1_meth;
    default:
      return NULL;
  }
}

void KEM_KEY_free(KEM_KEY *key) {
  if (key == NULL) {
    return;
  }
  KEM_KEY_clear(key);
  OPENSSL_free(key);
}

const KEM *KEM_KEY_get0_kem(KEM_KEY* key) {
  return key->kem;
}

const uint8_t *KEM_KEY_get0_secret_key(const KEM_KEY *key) {
  // Callers use a NULL return to test for the presence of a secret key, so a
  // NULL |key| must answer that question rather than crash.
  if (key == NULL) {
    return NULL;
  }
  return key->secret_key;
}

int KEM_KEY_set_raw_public_key(KEM_KEY *key, const uint8_t *in) {
  OPENSSL_free(key->public_key);
  key->public_key = OPENSSL_memdup(in, key->kem->public_key_len);
  if (key->public_key == NULL) {
    return 0;
  }

  return 1;
}

int KEM_KEY_set_raw_secret_key(KEM_KEY *key, const uint8_t *in) {
  OPENSSL_free(key->secret_key);
  key->secret_key = OPENSSL_memdup(in, key->kem->secret_key_len);
  if (key->secret_key == NULL) {
    return 0;
  }

  return 1;
}

int KEM_KEY_set_raw_key(KEM_KEY *key, const uint8_t *in_public,
                                      const uint8_t *in_secret) {
  OPENSSL_free(key->public_key);
  OPENSSL_free(key->secret_key);
  key->public_key = OPENSSL_memdup(in_public, key->kem->public_key_len);
  key->secret_key = OPENSSL_memdup(in_secret, key->kem->secret_key_len);
  if (key->public_key == NULL || key->secret_key == NULL) {
    KEM_KEY_clear(key);
    return 0;
  }

  return 1;
}

int KEM_KEY_set_raw_keypair_from_seed(KEM_KEY *key, const CBS *seed) {
  if (key == NULL || seed == NULL || key->kem == NULL) {
    OPENSSL_PUT_ERROR(CRYPTO, ERR_R_PASSED_NULL_PARAMETER);
    return 0;
  }

  // Ensure key is uninitialized
  if (key->public_key != NULL || key->secret_key != NULL) {
    OPENSSL_PUT_ERROR(CRYPTO, ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
    return 0;
  }

  // Validate seed length - all ML-KEM variants use 64-byte seeds
  if (CBS_len(seed) != key->kem->keygen_seed_len) {
    OPENSSL_PUT_ERROR(CRYPTO, ERR_R_OVERFLOW);
    return 0;
  }

  // Allocate buffers for key generation
  uint8_t *public_key = OPENSSL_malloc(key->kem->public_key_len);
  uint8_t *secret_key = OPENSSL_malloc(key->kem->secret_key_len);

  if (public_key == NULL || secret_key == NULL) {
    OPENSSL_free(public_key);
    OPENSSL_free(secret_key);
    return 0;
  }

  size_t public_len = key->kem->public_key_len;
  size_t secret_len = key->kem->secret_key_len;

  // Generate keypair from seed using the KEM method
  if (!key->kem->method->keygen_deterministic(public_key, &public_len,
                                              secret_key, &secret_len,
                                              CBS_data(seed))) {
    OPENSSL_PUT_ERROR(EVP, ERR_R_INTERNAL_ERROR);
    OPENSSL_free(public_key);
    OPENSSL_free(secret_key);
    return 0;
  }

  // Set public and secret key
  key->public_key = public_key;
  key->secret_key = secret_key;

  // Save a copy of the seed for seed-format encoding
  key->seed = OPENSSL_memdup(CBS_data(seed), key->kem->keygen_seed_len);
  if (key->seed == NULL) {
    KEM_KEY_clear(key);
    return 0;
  }

  return 1;
}

// The kem_check_* helpers below queue their own errors, so callers must not add
// another one on failure.

static int kem_check_public_key(const KEM_KEY *key) {
  int ok = 0;
  switch (key->kem->nid) {
    case NID_MLKEM512:
      ok = ml_kem_512_check_pk(key->public_key, key->kem->public_key_len) == 0;
      break;
    case NID_MLKEM768:
      ok = ml_kem_768_check_pk(key->public_key, key->kem->public_key_len) == 0;
      break;
    case NID_MLKEM1024:
      ok = ml_kem_1024_check_pk(key->public_key, key->kem->public_key_len) == 0;
      break;
    default:
      // Unreachable: KEM_KEY objects are only created for the NIDs above.
      OPENSSL_PUT_ERROR(EVP, ERR_R_INTERNAL_ERROR);
      return 0;
  }
  if (!ok) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_INVALID_PUBLIC_KEY);
    return 0;
  }
  return 1;
}

static int kem_check_secret_key(const KEM_KEY *key) {
  int ok = 0;
  switch (key->kem->nid) {
    case NID_MLKEM512:
      ok = ml_kem_512_check_sk(key->secret_key, key->kem->secret_key_len) == 0;
      break;
    case NID_MLKEM768:
      ok = ml_kem_768_check_sk(key->secret_key, key->kem->secret_key_len) == 0;
      break;
    case NID_MLKEM1024:
      ok = ml_kem_1024_check_sk(key->secret_key, key->kem->secret_key_len) == 0;
      break;
    default:
      // Unreachable: KEM_KEY objects are only created for the NIDs above.
      OPENSSL_PUT_ERROR(EVP, ERR_R_INTERNAL_ERROR);
      return 0;
  }
  if (!ok) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_INVALID_PRIVATE_KEY);
    return 0;
  }
  return 1;
}

static int kem_check_pct(const KEM_KEY *key) {
  int ret = 0;
  const KEM *kem = key->kem;

  uint8_t *ciphertext = OPENSSL_malloc(kem->ciphertext_len);
  uint8_t *ss_enc = OPENSSL_malloc(kem->shared_secret_len);
  uint8_t *ss_dec = OPENSSL_malloc(kem->shared_secret_len);

  size_t ct_len = kem->ciphertext_len;
  size_t ss_enc_len = kem->shared_secret_len;
  size_t ss_dec_len = kem->shared_secret_len;

  if (ciphertext == NULL || ss_enc == NULL || ss_dec == NULL) {
    // |OPENSSL_malloc| has already queued the allocation failure.
    goto cleanup;
  }

  // An encaps/decaps failure is a library fault, not an invalid key pair.
  if (!kem->method->encaps(ciphertext, &ct_len, ss_enc, &ss_enc_len,
                           key->public_key)) {
    OPENSSL_PUT_ERROR(EVP, ERR_R_INTERNAL_ERROR);
    goto cleanup;
  }

  // Validate the encapsulation output before feeding it back into decaps.
  if (ct_len != kem->ciphertext_len) {
    OPENSSL_PUT_ERROR(EVP, ERR_R_INTERNAL_ERROR);
    goto cleanup;
  }

  if (!kem->method->decaps(ss_dec, &ss_dec_len, ciphertext, key->secret_key)) {
    OPENSSL_PUT_ERROR(EVP, ERR_R_INTERNAL_ERROR);
    goto cleanup;
  }

  if (ss_enc_len != kem->shared_secret_len ||
      ss_dec_len != kem->shared_secret_len) {
    OPENSSL_PUT_ERROR(EVP, ERR_R_INTERNAL_ERROR);
    goto cleanup;
  }

  // Only a shared-secret mismatch means the key pair itself failed the PCT.
  if (CRYPTO_memcmp(ss_enc, ss_dec, kem->shared_secret_len) != 0) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_KEM_PCT_FAILED);
    goto cleanup;
  }

  ret = 1;

cleanup:
  // OPENSSL_free cleanses the buffer before releasing it and no-ops on NULL, so
  // neither an explicit OPENSSL_cleanse nor a NULL guard is needed here.
  OPENSSL_free(ciphertext);
  OPENSSL_free(ss_enc);
  OPENSSL_free(ss_dec);
  return ret;
}

int KEM_check_key(const KEM_KEY *key) {
  // Callers may reach the encaps/decaps methods below without going through
  // |EVP_PKEY_encapsulate|/|EVP_PKEY_decapsulate|, which is where DIT would
  // otherwise be established, so set it here.
  SET_DIT_AUTO_RESET;

  if (key == NULL || key->kem == NULL || key->kem->method == NULL) {
    OPENSSL_PUT_ERROR(EVP, ERR_R_PASSED_NULL_PARAMETER);
    return 0;
  }

  if (key->public_key == NULL) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_MISSING_PUBLIC_KEY);
    return 0;
  }

  int ret = 0;

  // The hash inside |kem_check_secret_key| and the encaps/decaps inside
  // |kem_check_pct| each update the service indicator, so validating one key
  // would otherwise bump it several times. Lock it for the duration and leave
  // the approval decision to the caller.
  FIPS_service_indicator_lock_state();

  if (!kem_check_public_key(key)) {
    goto end;
  }

  if (key->secret_key != NULL) {
    if (!kem_check_secret_key(key)) {
      goto end;
    }

    if (!kem_check_pct(key)) {
      goto end;
    }
  }

  ret = 1;

end:
  FIPS_service_indicator_unlock_state();
  return ret;
}
