// Copyright (c) 2020, Google Inc.
// SPDX-License-Identifier: ISC

#include <openssl/hpke.h>

#include <assert.h>
#include <string.h>

#include <openssl/aead.h>
#include <openssl/bytestring.h>
#include <openssl/curve25519.h>
#include <openssl/digest.h>
#include <openssl/err.h>
#include <openssl/evp_errors.h>
#include <openssl/hkdf.h>
#include <openssl/mem.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include "../internal.h"
#include "../fipsmodule/ml_kem/ml_kem.h"
#include "../fipsmodule/service_indicator/internal.h"


// This file implements RFC 9180 and draft-ietf-hpke-pq-05.

// Callers stack-allocate |EVP_HPKE_KEY|, so its size is ABI. Hold it at the
// size it had before the ML-KEM KEMs grew the key material: a KEM pointer plus
// the 32-byte X25519 private and public keys that used to sit inline.
OPENSSL_STATIC_ASSERT(sizeof(EVP_HPKE_KEY) == sizeof(void *) + 64,
                      evp_hpke_key_size_must_not_change)

// MAX_SEED_LEN is the largest |seed_len| of any KEM and MAX_SHARED_SECRET_LEN
// the largest Nsecret. Both are 32 for every KEM this file implements: X25519
// seeds an ephemeral private key with 32 bytes and ML-KEM takes 32 bytes of
// encapsulation entropy, and both produce a 32-byte shared secret.
#define MAX_SEED_LEN 32
#define MAX_SHARED_SECRET_LEN 32

struct evp_hpke_kem_st {
  uint16_t id;
  size_t public_key_len;
  size_t private_key_len;
  // expanded_private_key_len is the length of the KEM's internal private key
  // representation, or zero for KEMs which use the serialized private key
  // directly.
  size_t expanded_private_key_len;
  size_t seed_len;
  size_t enc_len;
  int (*init_key)(EVP_HPKE_KEY *key, const uint8_t *priv_key,
                  size_t priv_key_len);
  int (*generate_key)(EVP_HPKE_KEY *key);
  int (*encap_with_seed)(const EVP_HPKE_KEM *kem, uint8_t *out_shared_secret,
                         size_t *out_shared_secret_len, uint8_t *out_enc,
                         size_t *out_enc_len, size_t max_enc,
                         const uint8_t *peer_public_key,
                         size_t peer_public_key_len, const uint8_t *seed,
                         size_t seed_len);
  int (*decap)(const EVP_HPKE_KEY *key, uint8_t *out_shared_secret,
               size_t *out_shared_secret_len, const uint8_t *enc,
               size_t enc_len);
  int (*auth_encap_with_seed)(const EVP_HPKE_KEY *key,
                              uint8_t *out_shared_secret,
                              size_t *out_shared_secret_len, uint8_t *out_enc,
                              size_t *out_enc_len, size_t max_enc,
                              const uint8_t *peer_public_key,
                              size_t peer_public_key_len, const uint8_t *seed,
                              size_t seed_len);
  int (*auth_decap)(const EVP_HPKE_KEY *key, uint8_t *out_shared_secret,
                    size_t *out_shared_secret_len, const uint8_t *enc,
                    size_t enc_len, const uint8_t *peer_public_key,
                    size_t peer_public_key_len);
};

struct evp_hpke_kdf_st {
  uint16_t id;
  // We only support HKDF-based KDFs.
  const EVP_MD *(*hkdf_md_func)(void);
};

struct evp_hpke_aead_st {
  uint16_t id;
  const EVP_AEAD *(*aead_func)(void);
};


// Forward declarations for the implementations behind the public entry
// points at the end of this file.
static int hpke_key_init(EVP_HPKE_KEY *key, const EVP_HPKE_KEM *kem,
                         const uint8_t *priv_key, size_t priv_key_len);
static int hpke_key_generate(EVP_HPKE_KEY *key, const EVP_HPKE_KEM *kem);
static int hpke_ctx_setup_sender_with_seed_for_testing(
    EVP_HPKE_CTX *ctx, uint8_t *out_enc, size_t *out_enc_len, size_t max_enc,
    const EVP_HPKE_KEM *kem, const EVP_HPKE_KDF *kdf, const EVP_HPKE_AEAD *aead,
    const uint8_t *peer_public_key, size_t peer_public_key_len,
    const uint8_t *info, size_t info_len, const uint8_t *seed, size_t seed_len);
static int hpke_ctx_setup_sender(
    EVP_HPKE_CTX *ctx, uint8_t *out_enc, size_t *out_enc_len, size_t max_enc,
    const EVP_HPKE_KEM *kem, const EVP_HPKE_KDF *kdf, const EVP_HPKE_AEAD *aead,
    const uint8_t *peer_public_key, size_t peer_public_key_len,
    const uint8_t *info, size_t info_len);
static int hpke_ctx_setup_auth_sender_with_seed_for_testing(
    EVP_HPKE_CTX *ctx, uint8_t *out_enc, size_t *out_enc_len, size_t max_enc,
    const EVP_HPKE_KEY *key, const EVP_HPKE_KDF *kdf, const EVP_HPKE_AEAD *aead,
    const uint8_t *peer_public_key, size_t peer_public_key_len,
    const uint8_t *info, size_t info_len, const uint8_t *seed, size_t seed_len);
static int hpke_ctx_setup_auth_sender(
    EVP_HPKE_CTX *ctx, uint8_t *out_enc, size_t *out_enc_len, size_t max_enc,
    const EVP_HPKE_KEY *key, const EVP_HPKE_KDF *kdf, const EVP_HPKE_AEAD *aead,
    const uint8_t *peer_public_key, size_t peer_public_key_len,
    const uint8_t *info, size_t info_len);
static int hpke_ctx_setup_recipient(EVP_HPKE_CTX *ctx, const EVP_HPKE_KEY *key,
                                    const EVP_HPKE_KDF *kdf,
                                    const EVP_HPKE_AEAD *aead,
                                    const uint8_t *enc, size_t enc_len,
                                    const uint8_t *info, size_t info_len);
static int hpke_ctx_setup_auth_recipient(
    EVP_HPKE_CTX *ctx, const EVP_HPKE_KEY *key, const EVP_HPKE_KDF *kdf,
    const EVP_HPKE_AEAD *aead, const uint8_t *enc, size_t enc_len,
    const uint8_t *info, size_t info_len, const uint8_t *peer_public_key,
    size_t peer_public_key_len);
static int hpke_ctx_seal(EVP_HPKE_CTX *ctx, uint8_t *out, size_t *out_len,
                         size_t max_out_len, const uint8_t *in, size_t in_len,
                         const uint8_t *ad, size_t ad_len);
static int hpke_ctx_open(EVP_HPKE_CTX *ctx, uint8_t *out, size_t *out_len,
                         size_t max_out_len, const uint8_t *in, size_t in_len,
                         const uint8_t *ad, size_t ad_len);
static int hpke_ctx_export(const EVP_HPKE_CTX *ctx, uint8_t *out,
                           size_t secret_len, const uint8_t *context,
                           size_t context_len);

// Low-level labeled KDF functions.

static const char kHpkeVersionId[] = "HPKE-v1";

static int add_label_string(CBB *cbb, const char *label) {
  return CBB_add_bytes(cbb, (const uint8_t *)label, strlen(label));
}

static int hpke_labeled_extract(const EVP_MD *hkdf_md, uint8_t *out_key,
                                size_t *out_len, const uint8_t *salt,
                                size_t salt_len, const uint8_t *suite_id,
                                size_t suite_id_len, const char *label,
                                const uint8_t *ikm, size_t ikm_len) {
  // labeledIKM = concat("HPKE-v1", suite_id, label, IKM)
  CBB labeled_ikm;
  int ok = CBB_init(&labeled_ikm, 0) &&
           add_label_string(&labeled_ikm, kHpkeVersionId) &&
           CBB_add_bytes(&labeled_ikm, suite_id, suite_id_len) &&
           add_label_string(&labeled_ikm, label) &&
           CBB_add_bytes(&labeled_ikm, ikm, ikm_len) &&
           HKDF_extract(out_key, out_len, hkdf_md, CBB_data(&labeled_ikm),
                        CBB_len(&labeled_ikm), salt, salt_len);
  CBB_cleanup(&labeled_ikm);
  return ok;
}

static int hpke_labeled_expand(const EVP_MD *hkdf_md, uint8_t *out_key,
                               size_t out_len, const uint8_t *prk,
                               size_t prk_len, const uint8_t *suite_id,
                               size_t suite_id_len, const char *label,
                               const uint8_t *info, size_t info_len) {
  // labeledInfo = concat(I2OSP(L, 2), "HPKE-v1", suite_id, label, info)
  CBB labeled_info;
  int ok = CBB_init(&labeled_info, 0) &&
           CBB_add_u16(&labeled_info, out_len) &&
           add_label_string(&labeled_info, kHpkeVersionId) &&
           CBB_add_bytes(&labeled_info, suite_id, suite_id_len) &&
           add_label_string(&labeled_info, label) &&
           CBB_add_bytes(&labeled_info, info, info_len) &&
           HKDF_expand(out_key, out_len, hkdf_md, prk, prk_len,
                       CBB_data(&labeled_info), CBB_len(&labeled_info));
  CBB_cleanup(&labeled_info);
  return ok;
}


// KEM implementations.

// dhkem_extract_and_expand implements the ExtractAndExpand operation in the
// DHKEM construction. See section 4.1 of RFC 9180.
static int dhkem_extract_and_expand(uint16_t kem_id, const EVP_MD *hkdf_md,
                                    uint8_t *out_key, size_t out_len,
                                    const uint8_t *dh, size_t dh_len,
                                    const uint8_t *kem_context,
                                    size_t kem_context_len) {
  // concat("KEM", I2OSP(kem_id, 2))
  uint8_t suite_id[5] = {'K', 'E', 'M', kem_id >> 8, kem_id & 0xff};
  uint8_t prk[EVP_MAX_MD_SIZE];
  size_t prk_len;
  int ret = hpke_labeled_extract(hkdf_md, prk, &prk_len, NULL, 0, suite_id,
                              sizeof(suite_id), "eae_prk", dh, dh_len) &&
         hpke_labeled_expand(hkdf_md, out_key, out_len, prk, prk_len, suite_id,
                             sizeof(suite_id), "shared_secret", kem_context,
                             kem_context_len);
  OPENSSL_cleanse(prk, sizeof(prk));
  return ret;
}

static int x25519_init_key(EVP_HPKE_KEY *key, const uint8_t *priv_key,
                           size_t priv_key_len) {
  if (priv_key_len != X25519_PRIVATE_KEY_LEN) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_DECODE_ERROR);
    return 0;
  }

  OPENSSL_memcpy(key->private_key, priv_key, priv_key_len);
  X25519_public_from_private(key->public_key, priv_key);
  return 1;
}

static int x25519_generate_key(EVP_HPKE_KEY *key) {
  X25519_keypair(key->public_key, key->private_key);
  return 1;
}

static int x25519_encap_with_seed(
    const EVP_HPKE_KEM *kem, uint8_t *out_shared_secret,
    size_t *out_shared_secret_len, uint8_t *out_enc, size_t *out_enc_len,
    size_t max_enc, const uint8_t *peer_public_key, size_t peer_public_key_len,
    const uint8_t *seed, size_t seed_len) {
  if (max_enc < X25519_PUBLIC_VALUE_LEN) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_INVALID_BUFFER_SIZE);
    return 0;
  }
  if (seed_len != X25519_PRIVATE_KEY_LEN) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_DECODE_ERROR);
    return 0;
  }
  X25519_public_from_private(out_enc, seed);

  uint8_t dh[X25519_SHARED_KEY_LEN];
  if (peer_public_key_len != X25519_PUBLIC_VALUE_LEN ||
      !X25519(dh, seed, peer_public_key)) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_INVALID_PEER_KEY);
    return 0;
  }

  uint8_t kem_context[2 * X25519_PUBLIC_VALUE_LEN];
  OPENSSL_memcpy(kem_context, out_enc, X25519_PUBLIC_VALUE_LEN);
  OPENSSL_memcpy(kem_context + X25519_PUBLIC_VALUE_LEN, peer_public_key,
                 X25519_PUBLIC_VALUE_LEN);
  if (!dhkem_extract_and_expand(kem->id, EVP_sha256(), out_shared_secret,
                                SHA256_DIGEST_LENGTH, dh, sizeof(dh),
                                kem_context, sizeof(kem_context))) {
    return 0;
  }

  *out_enc_len = X25519_PUBLIC_VALUE_LEN;
  *out_shared_secret_len = SHA256_DIGEST_LENGTH;
  return 1;
}

static int x25519_decap(const EVP_HPKE_KEY *key, uint8_t *out_shared_secret,
                        size_t *out_shared_secret_len, const uint8_t *enc,
                        size_t enc_len) {
  uint8_t dh[X25519_SHARED_KEY_LEN];
  if (enc_len != X25519_PUBLIC_VALUE_LEN ||
      !X25519(dh, key->private_key, enc)) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_INVALID_PEER_KEY);
    return 0;
  }

  uint8_t kem_context[2 * X25519_PUBLIC_VALUE_LEN];
  OPENSSL_memcpy(kem_context, enc, X25519_PUBLIC_VALUE_LEN);
  OPENSSL_memcpy(kem_context + X25519_PUBLIC_VALUE_LEN, key->public_key,
                 X25519_PUBLIC_VALUE_LEN);
  if (!dhkem_extract_and_expand(key->kem->id, EVP_sha256(), out_shared_secret,
                                SHA256_DIGEST_LENGTH, dh, sizeof(dh),
                                kem_context, sizeof(kem_context))) {
    return 0;
  }

  *out_shared_secret_len = SHA256_DIGEST_LENGTH;
  return 1;
}

static int x25519_auth_encap_with_seed(
    const EVP_HPKE_KEY *key, uint8_t *out_shared_secret,
    size_t *out_shared_secret_len, uint8_t *out_enc, size_t *out_enc_len,
    size_t max_enc, const uint8_t *peer_public_key, size_t peer_public_key_len,
    const uint8_t *seed, size_t seed_len) {
  if (max_enc < X25519_PUBLIC_VALUE_LEN) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_INVALID_BUFFER_SIZE);
    return 0;
  }
  if (seed_len != X25519_PRIVATE_KEY_LEN) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_DECODE_ERROR);
    return 0;
  }
  X25519_public_from_private(out_enc, seed);

  uint8_t dh[2 * X25519_SHARED_KEY_LEN];
  if (peer_public_key_len != X25519_PUBLIC_VALUE_LEN ||
      !X25519(dh, seed, peer_public_key) ||
      !X25519(dh + X25519_SHARED_KEY_LEN, key->private_key, peer_public_key)) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_INVALID_PEER_KEY);
    return 0;
  }

  uint8_t kem_context[3 * X25519_PUBLIC_VALUE_LEN];
  OPENSSL_memcpy(kem_context, out_enc, X25519_PUBLIC_VALUE_LEN);
  OPENSSL_memcpy(kem_context + X25519_PUBLIC_VALUE_LEN, peer_public_key,
                 X25519_PUBLIC_VALUE_LEN);
  OPENSSL_memcpy(kem_context + 2 * X25519_PUBLIC_VALUE_LEN, key->public_key,
                 X25519_PUBLIC_VALUE_LEN);
  if (!dhkem_extract_and_expand(key->kem->id, EVP_sha256(), out_shared_secret,
                                SHA256_DIGEST_LENGTH, dh, sizeof(dh),
                                kem_context, sizeof(kem_context))) {
    return 0;
  }

  *out_enc_len = X25519_PUBLIC_VALUE_LEN;
  *out_shared_secret_len = SHA256_DIGEST_LENGTH;
  return 1;
}

static int x25519_auth_decap(const EVP_HPKE_KEY *key,
                             uint8_t *out_shared_secret,
                             size_t *out_shared_secret_len, const uint8_t *enc,
                             size_t enc_len, const uint8_t *peer_public_key,
                             size_t peer_public_key_len) {
  uint8_t dh[2 * X25519_SHARED_KEY_LEN];
  if (enc_len != X25519_PUBLIC_VALUE_LEN ||
      peer_public_key_len != X25519_PUBLIC_VALUE_LEN ||
      !X25519(dh, key->private_key, enc) ||
      !X25519(dh + X25519_SHARED_KEY_LEN, key->private_key, peer_public_key)) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_INVALID_PEER_KEY);
    return 0;
  }

  uint8_t kem_context[3 * X25519_PUBLIC_VALUE_LEN];
  OPENSSL_memcpy(kem_context, enc, X25519_PUBLIC_VALUE_LEN);
  OPENSSL_memcpy(kem_context + X25519_PUBLIC_VALUE_LEN, key->public_key,
                 X25519_PUBLIC_VALUE_LEN);
  OPENSSL_memcpy(kem_context + 2 * X25519_PUBLIC_VALUE_LEN, peer_public_key,
                 X25519_PUBLIC_VALUE_LEN);
  if (!dhkem_extract_and_expand(key->kem->id, EVP_sha256(), out_shared_secret,
                                SHA256_DIGEST_LENGTH, dh, sizeof(dh),
                                kem_context, sizeof(kem_context))) {
    return 0;
  }

  *out_shared_secret_len = SHA256_DIGEST_LENGTH;
  return 1;
}

const EVP_HPKE_KEM *EVP_hpke_x25519_hkdf_sha256(void) {
  static const EVP_HPKE_KEM kKEM = {
      /*id=*/EVP_HPKE_DHKEM_X25519_HKDF_SHA256,
      /*public_key_len=*/X25519_PUBLIC_VALUE_LEN,
      /*private_key_len=*/X25519_PRIVATE_KEY_LEN,
      /*expanded_private_key_len=*/0,
      /*seed_len=*/X25519_PRIVATE_KEY_LEN,
      /*enc_len=*/X25519_PUBLIC_VALUE_LEN,
      x25519_init_key,
      x25519_generate_key,
      x25519_encap_with_seed,
      x25519_decap,
      x25519_auth_encap_with_seed,
      x25519_auth_decap,
  };
  return &kKEM;
}


// ML-KEM KEM implementations (draft-ietf-hpke-pq-05).
//
// Per section 3 of the draft, an ML-KEM private key is serialized as the
// 64-byte (d || z) seed, not as the expanded decapsulation key returned by
// ML-KEM.KeyGen, so Nsk is 64 for every parameter set. |EVP_HPKE_KEY| stores
// the seed as the private key and additionally caches the expanded
// decapsulation key, which is derived once when the key is initialized.
//
// Caching it, rather than re-deriving per decapsulation, is deliberate. The only
// ML-KEM entry point which expands a seed is key generation, and in FIPS builds
// that runs a pairwise consistency test with fatal module-failure semantics and
// a DRBG draw. Re-deriving per decapsulation would put all of that on a path
// reached from the network. See |mlkem_decap|.
//
// Encap and Decap correspond directly to ML-KEM.Encaps and ML-KEM.Decaps. The
// ML-KEM shared secret is used as the HPKE shared secret with no
// ExtractAndExpand step, so Nsecret is 32 for every parameter set.
//
// ML-KEM does not support AuthEncap or AuthDecap, so the auth hooks are NULL
// and the mode_auth entry points report an error. See section 7.2 of the draft.

#define MLKEM_SEED_LEN 64
#define MLKEM_SHARED_SECRET_LEN 32

OPENSSL_STATIC_ASSERT(MLKEM_SEED_LEN == MLKEM512_KEYGEN_SEED_LEN &&
                          MLKEM_SEED_LEN == MLKEM768_KEYGEN_SEED_LEN &&
                          MLKEM_SEED_LEN == MLKEM1024_KEYGEN_SEED_LEN,
                      ml_kem_keygen_seed_is_not_64_bytes)
OPENSSL_STATIC_ASSERT(MLKEM_SEED_LEN <= EVP_HPKE_MAX_PRIVATE_KEY_LENGTH,
                      evp_hpke_key_private_key_too_small_for_ml_kem)
OPENSSL_STATIC_ASSERT(MLKEM1024_PUBLIC_KEY_BYTES <=
                          EVP_HPKE_MAX_PUBLIC_KEY_LENGTH,
                      evp_hpke_key_public_key_too_small_for_ml_kem)
OPENSSL_STATIC_ASSERT(MLKEM1024_CIPHERTEXT_BYTES <= EVP_HPKE_MAX_ENC_LENGTH,
                      evp_hpke_max_enc_length_too_small_for_ml_kem)
OPENSSL_STATIC_ASSERT(MLKEM_SHARED_SECRET_LEN <= MAX_SHARED_SECRET_LEN,
                      max_shared_secret_len_too_small_for_ml_kem)
OPENSSL_STATIC_ASSERT(MLKEM512_ENCAPS_SEED_LEN <= MAX_SEED_LEN &&
                          MLKEM768_ENCAPS_SEED_LEN <= MAX_SEED_LEN &&
                          MLKEM1024_ENCAPS_SEED_LEN <= MAX_SEED_LEN,
                      max_seed_len_too_small_for_ml_kem)

// MLKEM_METHOD abstracts over the three ML-KEM parameter sets so that the HPKE
// glue below can be written once.
typedef struct {
  size_t public_key_len;
  size_t expanded_private_key_len;
  size_t enc_len;
  size_t encaps_seed_len;
  int (*keypair_deterministic)(uint8_t *public_key, size_t *public_len,
                               uint8_t *secret_key, size_t *secret_len,
                               const uint8_t *seed);
  int (*encapsulate_deterministic)(uint8_t *ciphertext, size_t *ciphertext_len,
                                   uint8_t *shared_secret,
                                   size_t *shared_secret_len,
                                   const uint8_t *public_key,
                                   const uint8_t *seed);
  int (*decapsulate)(uint8_t *shared_secret, size_t *shared_secret_len,
                     const uint8_t *ciphertext, const uint8_t *secret_key);
  int (*check_pk)(const uint8_t *public_key, size_t public_key_len);
} MLKEM_METHOD;

static const MLKEM_METHOD kMLKEM512Method = {
    /*public_key_len=*/MLKEM512_PUBLIC_KEY_BYTES,
    /*expanded_private_key_len=*/MLKEM512_SECRET_KEY_BYTES,
    /*enc_len=*/MLKEM512_CIPHERTEXT_BYTES,
    /*encaps_seed_len=*/MLKEM512_ENCAPS_SEED_LEN,
    ml_kem_512_keypair_deterministic,
    ml_kem_512_encapsulate_deterministic,
    ml_kem_512_decapsulate,
    ml_kem_512_check_pk,
};

static const MLKEM_METHOD kMLKEM768Method = {
    /*public_key_len=*/MLKEM768_PUBLIC_KEY_BYTES,
    /*expanded_private_key_len=*/MLKEM768_SECRET_KEY_BYTES,
    /*enc_len=*/MLKEM768_CIPHERTEXT_BYTES,
    /*encaps_seed_len=*/MLKEM768_ENCAPS_SEED_LEN,
    ml_kem_768_keypair_deterministic,
    ml_kem_768_encapsulate_deterministic,
    ml_kem_768_decapsulate,
    ml_kem_768_check_pk,
};

static const MLKEM_METHOD kMLKEM1024Method = {
    /*public_key_len=*/MLKEM1024_PUBLIC_KEY_BYTES,
    /*expanded_private_key_len=*/MLKEM1024_SECRET_KEY_BYTES,
    /*enc_len=*/MLKEM1024_CIPHERTEXT_BYTES,
    /*encaps_seed_len=*/MLKEM1024_ENCAPS_SEED_LEN,
    ml_kem_1024_keypair_deterministic,
    ml_kem_1024_encapsulate_deterministic,
    ml_kem_1024_decapsulate,
    ml_kem_1024_check_pk,
};

// mlkem_expand_seed_into_key derives |meth|'s key pair from the
// |MLKEM_SEED_LEN|-byte |seed| directly into |key|. Both outputs of the ML-KEM
// key generation entry point are wanted, so no scratch buffer is needed, and
// deriving here once means decapsulation never has to repeat it.
static int mlkem_expand_seed_into_key(EVP_HPKE_KEY *key,
                                      const MLKEM_METHOD *meth,
                                      const uint8_t *seed) {
  // The capacities come from |key->kem|, which sized the allocation, so that a
  // disagreement with |meth| fails the KEM's own length check rather than
  // overrunning the buffer.
  size_t public_key_len = key->kem->public_key_len;
  size_t expanded_private_key_len = key->kem->expanded_private_key_len;
  if (meth->keypair_deterministic(key->public_key, &public_key_len,
                                  key->expanded_private_key,
                                  &expanded_private_key_len, seed) != 0) {
    OPENSSL_PUT_ERROR(EVP, ERR_R_INTERNAL_ERROR);
    return 0;
  }
  return 1;
}

static int mlkem_init_key(EVP_HPKE_KEY *key, const MLKEM_METHOD *meth,
                          const uint8_t *priv_key, size_t priv_key_len) {
  // Every 64-byte string is a valid ML-KEM seed, so there is nothing to check
  // beyond the length; the expanded key is correct by construction.
  if (priv_key_len != MLKEM_SEED_LEN) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_DECODE_ERROR);
    return 0;
  }
  if (!mlkem_expand_seed_into_key(key, meth, priv_key)) {
    return 0;
  }
  OPENSSL_memcpy(key->private_key, priv_key, MLKEM_SEED_LEN);
  return 1;
}

static int mlkem_generate_key(EVP_HPKE_KEY *key, const MLKEM_METHOD *meth) {
  uint8_t seed[MLKEM_SEED_LEN];
  AWSLC_ABORT_IF_NOT_ONE(RAND_bytes(seed, sizeof(seed)));
  int ret = mlkem_init_key(key, meth, seed, sizeof(seed));
  OPENSSL_cleanse(seed, sizeof(seed));
  return ret;
}

static int mlkem_encap_with_seed(const MLKEM_METHOD *meth,
                                 uint8_t *out_shared_secret,
                                 size_t *out_shared_secret_len,
                                 uint8_t *out_enc, size_t *out_enc_len,
                                 size_t max_enc, const uint8_t *peer_public_key,
                                 size_t peer_public_key_len,
                                 const uint8_t *seed, size_t seed_len) {
  if (max_enc < meth->enc_len) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_INVALID_BUFFER_SIZE);
    return 0;
  }
  if (seed_len != meth->encaps_seed_len) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_DECODE_ERROR);
    return 0;
  }
  // The draft requires that an ML-KEM encapsulation key check failure be
  // reported as an HPKE EncapError. |encapsulate_deterministic| does not
  // surface the key check separately, so check the key here.
  if (peer_public_key_len != meth->public_key_len ||
      meth->check_pk(peer_public_key, peer_public_key_len) != 0) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_INVALID_PEER_KEY);
    return 0;
  }

  size_t enc_len = meth->enc_len;
  size_t shared_secret_len = MLKEM_SHARED_SECRET_LEN;
  if (meth->encapsulate_deterministic(out_enc, &enc_len, out_shared_secret,
                                      &shared_secret_len, peer_public_key,
                                      seed) != 0) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_INVALID_PEER_KEY);
    return 0;
  }

  *out_enc_len = enc_len;
  *out_shared_secret_len = shared_secret_len;
  return 1;
}

static int mlkem_decap(const EVP_HPKE_KEY *key, const MLKEM_METHOD *meth,
                       uint8_t *out_shared_secret,
                       size_t *out_shared_secret_len, const uint8_t *enc,
                       size_t enc_len) {
  if (enc_len != meth->enc_len) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_INVALID_PEER_KEY);
    return 0;
  }

  // ML-KEM decapsulation is implicitly rejecting: a ciphertext which does not
  // decrypt correctly yields a pseudorandom shared secret rather than an error,
  // so a failure here means the input was malformed.
  //
  // The expanded decapsulation key was derived when |key| was initialized, so
  // this path does no key generation. That matters in FIPS builds, where the
  // ML-KEM key generation entry point also runs a pairwise consistency test:
  // running that per decapsulation would put a key generation health test, with
  // fatal module-failure semantics, on a path reached from the network.
  size_t shared_secret_len = MLKEM_SHARED_SECRET_LEN;
  if (meth->decapsulate(out_shared_secret, &shared_secret_len, enc,
                        key->expanded_private_key) != 0) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_INVALID_PEER_KEY);
    return 0;
  }

  *out_shared_secret_len = shared_secret_len;
  return 1;
}


static int mlkem512_init_key(EVP_HPKE_KEY *key, const uint8_t *priv_key,
                             size_t priv_key_len) {
  return mlkem_init_key(key, &kMLKEM512Method, priv_key, priv_key_len);
}

static int mlkem512_generate_key(EVP_HPKE_KEY *key) {
  return mlkem_generate_key(key, &kMLKEM512Method);
}

static int mlkem512_encap_with_seed(
    const EVP_HPKE_KEM *kem, uint8_t *out_shared_secret,
    size_t *out_shared_secret_len, uint8_t *out_enc, size_t *out_enc_len,
    size_t max_enc, const uint8_t *peer_public_key, size_t peer_public_key_len,
    const uint8_t *seed, size_t seed_len) {
  return mlkem_encap_with_seed(&kMLKEM512Method, out_shared_secret,
                               out_shared_secret_len, out_enc, out_enc_len,
                               max_enc, peer_public_key, peer_public_key_len,
                               seed, seed_len);
}

static int mlkem512_decap(const EVP_HPKE_KEY *key, uint8_t *out_shared_secret,
                          size_t *out_shared_secret_len, const uint8_t *enc,
                          size_t enc_len) {
  return mlkem_decap(key, &kMLKEM512Method, out_shared_secret,
                     out_shared_secret_len, enc, enc_len);
}

const EVP_HPKE_KEM *EVP_hpke_mlkem512(void) {
  static const EVP_HPKE_KEM kKEM = {
      /*id=*/EVP_HPKE_MLKEM512,
      /*public_key_len=*/MLKEM512_PUBLIC_KEY_BYTES,
      /*private_key_len=*/MLKEM_SEED_LEN,
      /*expanded_private_key_len=*/MLKEM512_SECRET_KEY_BYTES,
      /*seed_len=*/MLKEM512_ENCAPS_SEED_LEN,
      /*enc_len=*/MLKEM512_CIPHERTEXT_BYTES,
      mlkem512_init_key,
      mlkem512_generate_key,
      mlkem512_encap_with_seed,
      mlkem512_decap,
      /*auth_encap_with_seed=*/NULL,
      /*auth_decap=*/NULL,
  };
  return &kKEM;
}


static int mlkem768_init_key(EVP_HPKE_KEY *key, const uint8_t *priv_key,
                             size_t priv_key_len) {
  return mlkem_init_key(key, &kMLKEM768Method, priv_key, priv_key_len);
}

static int mlkem768_generate_key(EVP_HPKE_KEY *key) {
  return mlkem_generate_key(key, &kMLKEM768Method);
}

static int mlkem768_encap_with_seed(
    const EVP_HPKE_KEM *kem, uint8_t *out_shared_secret,
    size_t *out_shared_secret_len, uint8_t *out_enc, size_t *out_enc_len,
    size_t max_enc, const uint8_t *peer_public_key, size_t peer_public_key_len,
    const uint8_t *seed, size_t seed_len) {
  return mlkem_encap_with_seed(&kMLKEM768Method, out_shared_secret,
                               out_shared_secret_len, out_enc, out_enc_len,
                               max_enc, peer_public_key, peer_public_key_len,
                               seed, seed_len);
}

static int mlkem768_decap(const EVP_HPKE_KEY *key, uint8_t *out_shared_secret,
                          size_t *out_shared_secret_len, const uint8_t *enc,
                          size_t enc_len) {
  return mlkem_decap(key, &kMLKEM768Method, out_shared_secret,
                     out_shared_secret_len, enc, enc_len);
}

const EVP_HPKE_KEM *EVP_hpke_mlkem768(void) {
  static const EVP_HPKE_KEM kKEM = {
      /*id=*/EVP_HPKE_MLKEM768,
      /*public_key_len=*/MLKEM768_PUBLIC_KEY_BYTES,
      /*private_key_len=*/MLKEM_SEED_LEN,
      /*expanded_private_key_len=*/MLKEM768_SECRET_KEY_BYTES,
      /*seed_len=*/MLKEM768_ENCAPS_SEED_LEN,
      /*enc_len=*/MLKEM768_CIPHERTEXT_BYTES,
      mlkem768_init_key,
      mlkem768_generate_key,
      mlkem768_encap_with_seed,
      mlkem768_decap,
      /*auth_encap_with_seed=*/NULL,
      /*auth_decap=*/NULL,
  };
  return &kKEM;
}


static int mlkem1024_init_key(EVP_HPKE_KEY *key, const uint8_t *priv_key,
                              size_t priv_key_len) {
  return mlkem_init_key(key, &kMLKEM1024Method, priv_key, priv_key_len);
}

static int mlkem1024_generate_key(EVP_HPKE_KEY *key) {
  return mlkem_generate_key(key, &kMLKEM1024Method);
}

static int mlkem1024_encap_with_seed(
    const EVP_HPKE_KEM *kem, uint8_t *out_shared_secret,
    size_t *out_shared_secret_len, uint8_t *out_enc, size_t *out_enc_len,
    size_t max_enc, const uint8_t *peer_public_key, size_t peer_public_key_len,
    const uint8_t *seed, size_t seed_len) {
  return mlkem_encap_with_seed(&kMLKEM1024Method, out_shared_secret,
                               out_shared_secret_len, out_enc, out_enc_len,
                               max_enc, peer_public_key, peer_public_key_len,
                               seed, seed_len);
}

static int mlkem1024_decap(const EVP_HPKE_KEY *key, uint8_t *out_shared_secret,
                           size_t *out_shared_secret_len, const uint8_t *enc,
                           size_t enc_len) {
  return mlkem_decap(key, &kMLKEM1024Method, out_shared_secret,
                     out_shared_secret_len, enc, enc_len);
}

const EVP_HPKE_KEM *EVP_hpke_mlkem1024(void) {
  static const EVP_HPKE_KEM kKEM = {
      /*id=*/EVP_HPKE_MLKEM1024,
      /*public_key_len=*/MLKEM1024_PUBLIC_KEY_BYTES,
      /*private_key_len=*/MLKEM_SEED_LEN,
      /*expanded_private_key_len=*/MLKEM1024_SECRET_KEY_BYTES,
      /*seed_len=*/MLKEM1024_ENCAPS_SEED_LEN,
      /*enc_len=*/MLKEM1024_CIPHERTEXT_BYTES,
      mlkem1024_init_key,
      mlkem1024_generate_key,
      mlkem1024_encap_with_seed,
      mlkem1024_decap,
      /*auth_encap_with_seed=*/NULL,
      /*auth_decap=*/NULL,
  };
  return &kKEM;
}

uint16_t EVP_HPKE_KEM_id(const EVP_HPKE_KEM *kem) { return kem->id; }

size_t EVP_HPKE_KEM_public_key_len(const EVP_HPKE_KEM *kem) {
  return kem->public_key_len;
}

size_t EVP_HPKE_KEM_private_key_len(const EVP_HPKE_KEM *kem) {
  return kem->private_key_len;
}

size_t EVP_HPKE_KEM_enc_len(const EVP_HPKE_KEM *kem) { return kem->enc_len; }

// hpke_key_buffer_len returns the size of the single allocation backing a key
// for |kem|, which is partitioned by |hpke_key_alloc|.
static size_t hpke_key_buffer_len(const EVP_HPKE_KEM *kem) {
  return kem->private_key_len + kem->public_key_len +
         kem->expanded_private_key_len;
}

// hpke_key_alloc points |key|'s buffers into one allocation sized for
// |key->kem|, which must already be set. It returns one on success and zero on
// allocation failure, after which the buffers are left NULL.
static int hpke_key_alloc(EVP_HPKE_KEY *key) {
  const EVP_HPKE_KEM *kem = key->kem;
  uint8_t *buf = OPENSSL_zalloc(hpke_key_buffer_len(kem));
  if (buf == NULL) {
    return 0;
  }
  // |private_key| owns the allocation, so it comes first and is what
  // |hpke_key_free| frees.
  key->private_key = buf;
  key->public_key = buf + kem->private_key_len;
  key->expanded_private_key =
      kem->expanded_private_key_len == 0
          ? NULL
          : buf + kem->private_key_len + kem->public_key_len;
  return 1;
}

// hpke_key_free cleanses and releases |key|'s buffers, leaving the pointers
// dangling for the caller to clear.
static void hpke_key_free(EVP_HPKE_KEY *key) {
  if (key->private_key == NULL) {
    return;
  }
  // Cleanse with a barrier, which |OPENSSL_free| does not guarantee for the
  // whole buffer and which a plain memset does not guarantee at all.
  OPENSSL_cleanse(key->private_key, hpke_key_buffer_len(key->kem));
  OPENSSL_free(key->private_key);
}

void EVP_HPKE_KEY_zero(EVP_HPKE_KEY *key) {
  OPENSSL_memset(key, 0, sizeof(EVP_HPKE_KEY));
}

void EVP_HPKE_KEY_cleanup(EVP_HPKE_KEY *key) {
  if (key == NULL) {
    return;
  }
  // Return the struct to the zero state so that the result is indistinguishable
  // from a key which has only ever been passed to |EVP_HPKE_KEY_zero|.
  //
  // Clearing |kem| in particular means that using |key| after cleanup fails,
  // rather than silently operating on whatever key the all-zero private key
  // describes: every 64-byte string is a valid ML-KEM seed, and a zero X25519
  // scalar is clamped to a valid one, so a cleansed key would otherwise still
  // "work".
  hpke_key_free(key);
  EVP_HPKE_KEY_zero(key);
}

EVP_HPKE_KEY *EVP_HPKE_KEY_new(void) {
  EVP_HPKE_KEY *key = OPENSSL_malloc(sizeof(EVP_HPKE_KEY));
  if (key == NULL) {
    return NULL;
  }
  EVP_HPKE_KEY_zero(key);
  return key;
}

void EVP_HPKE_KEY_free(EVP_HPKE_KEY *key) {
  if (key != NULL) {
    EVP_HPKE_KEY_cleanup(key);
    OPENSSL_free(key);
  }
}

int EVP_HPKE_KEY_copy(EVP_HPKE_KEY *dst, const EVP_HPKE_KEY *src) {
  if (dst == src) {
    // |memcpy| requires non-overlapping regions, so self-copy would be
    // undefined. There is also nothing to do.
    return 1;
  }
  // As in |hpke_key_init|, |dst| may be uninitialized. Copying onto a live key
  // requires |EVP_HPKE_KEY_cleanup| first.
  EVP_HPKE_KEY_zero(dst);
  if (src->kem == NULL) {
    // |src| is in the zero state, which |dst| now matches.
    return 1;
  }
  dst->kem = src->kem;
  if (!hpke_key_alloc(dst)) {
    EVP_HPKE_KEY_cleanup(dst);
    return 0;
  }
  OPENSSL_memcpy(dst->private_key, src->private_key,
                 hpke_key_buffer_len(src->kem));
  return 1;
}

void EVP_HPKE_KEY_move(EVP_HPKE_KEY *out, EVP_HPKE_KEY *in) {
  if (out == in) {
    // As in |EVP_HPKE_KEY_copy|, self-move would be an overlapping |memcpy|.
    // Moving a key onto itself leaves it as it was, rather than zeroing it.
    return;
  }
  EVP_HPKE_KEY_cleanup(out);
  OPENSSL_memcpy(out, in, sizeof(EVP_HPKE_KEY));
  // |out| now owns the buffers, so |in| is only cleared. Cleansing it here
  // would erase and free key material |out| still points at.
  EVP_HPKE_KEY_zero(in);
}

static int hpke_key_init(EVP_HPKE_KEY *key, const EVP_HPKE_KEM *kem,
                         const uint8_t *priv_key, size_t priv_key_len) {
  // |key| may be uninitialized, so zero it rather than clean it up. Every
  // release has accepted a stack |EVP_HPKE_KEY| passed straight to this
  // function, and cleaning up would free whatever the stack bytes hold.
  // Re-initializing a live key requires |EVP_HPKE_KEY_cleanup| first.
  EVP_HPKE_KEY_zero(key);
  key->kem = kem;
  if (!hpke_key_alloc(key) || !kem->init_key(key, priv_key, priv_key_len)) {
    // |init_key| may have failed partway and left key material behind, so
    // cleanse rather than only clearing |kem|.
    EVP_HPKE_KEY_cleanup(key);
    return 0;
  }
  return 1;
}

static int hpke_key_generate(EVP_HPKE_KEY *key, const EVP_HPKE_KEM *kem) {
  // See |hpke_key_init| for why |key| is zeroed rather than cleaned up.
  EVP_HPKE_KEY_zero(key);
  key->kem = kem;
  if (!hpke_key_alloc(key) || !kem->generate_key(key)) {
    // As in |hpke_key_init|, cleanse in case |generate_key| failed partway.
    EVP_HPKE_KEY_cleanup(key);
    return 0;
  }
  return 1;
}

const EVP_HPKE_KEM *EVP_HPKE_KEY_kem(const EVP_HPKE_KEY *key) {
  return key->kem;
}

int EVP_HPKE_KEY_public_key(const EVP_HPKE_KEY *key, uint8_t *out,
                            size_t *out_len, size_t max_out) {
  // A zeroed or cleaned-up key has no KEM; fail rather than dereference NULL.
  if (key->kem == NULL) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_NO_KEY_SET);
    return 0;
  }
  if (max_out < key->kem->public_key_len) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_INVALID_BUFFER_SIZE);
    return 0;
  }
  OPENSSL_memcpy(out, key->public_key, key->kem->public_key_len);
  *out_len = key->kem->public_key_len;
  return 1;
}

int EVP_HPKE_KEY_private_key(const EVP_HPKE_KEY *key, uint8_t *out,
                            size_t *out_len, size_t max_out) {
  if (key->kem == NULL) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_NO_KEY_SET);
    return 0;
  }
  if (max_out < key->kem->private_key_len) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_INVALID_BUFFER_SIZE);
    return 0;
  }
  OPENSSL_memcpy(out, key->private_key, key->kem->private_key_len);
  *out_len = key->kem->private_key_len;
  return 1;
}


// Supported KDFs and AEADs.

const EVP_HPKE_KDF *EVP_hpke_hkdf_sha256(void) {
  static const EVP_HPKE_KDF kKDF = {EVP_HPKE_HKDF_SHA256, &EVP_sha256};
  return &kKDF;
}

const EVP_HPKE_KDF *EVP_hpke_hkdf_sha384(void) {
  static const EVP_HPKE_KDF kKDF = {EVP_HPKE_HKDF_SHA384, &EVP_sha384};
  return &kKDF;
}

uint16_t EVP_HPKE_KDF_id(const EVP_HPKE_KDF *kdf) { return kdf->id; }

const EVP_MD *EVP_HPKE_KDF_hkdf_md(const EVP_HPKE_KDF *kdf) {
  return kdf->hkdf_md_func();
}

const EVP_HPKE_AEAD *EVP_hpke_aes_128_gcm(void) {
  static const EVP_HPKE_AEAD kAEAD = {EVP_HPKE_AES_128_GCM,
                                      &EVP_aead_aes_128_gcm};
  return &kAEAD;
}

const EVP_HPKE_AEAD *EVP_hpke_aes_256_gcm(void) {
  static const EVP_HPKE_AEAD kAEAD = {EVP_HPKE_AES_256_GCM,
                                      &EVP_aead_aes_256_gcm};
  return &kAEAD;
}

const EVP_HPKE_AEAD *EVP_hpke_chacha20_poly1305(void) {
  static const EVP_HPKE_AEAD kAEAD = {EVP_HPKE_CHACHA20_POLY1305,
                                      &EVP_aead_chacha20_poly1305};
  return &kAEAD;
}

uint16_t EVP_HPKE_AEAD_id(const EVP_HPKE_AEAD *aead) { return aead->id; }

const EVP_AEAD *EVP_HPKE_AEAD_aead(const EVP_HPKE_AEAD *aead) {
  return aead->aead_func();
}


// HPKE implementation.

// This is strlen("HPKE") + 3 * sizeof(uint16_t).
#define HPKE_SUITE_ID_LEN 10

// The suite_id for non-KEM pieces of HPKE is defined as concat("HPKE",
// I2OSP(kem_id, 2), I2OSP(kdf_id, 2), I2OSP(aead_id, 2)).
static int hpke_build_suite_id(const EVP_HPKE_CTX *ctx,
                               uint8_t out[HPKE_SUITE_ID_LEN]) {
  CBB cbb;
  CBB_init_fixed(&cbb, out, HPKE_SUITE_ID_LEN);
  return add_label_string(&cbb, "HPKE") &&   //
         CBB_add_u16(&cbb, ctx->kem->id) &&  //
         CBB_add_u16(&cbb, ctx->kdf->id) &&  //
         CBB_add_u16(&cbb, ctx->aead->id);
}

#define HPKE_MODE_BASE 0
#define HPKE_MODE_AUTH 2

static int hpke_key_schedule(EVP_HPKE_CTX *ctx, uint8_t mode,
                             const uint8_t *shared_secret,
                             size_t shared_secret_len, const uint8_t *info,
                             size_t info_len) {
  uint8_t suite_id[HPKE_SUITE_ID_LEN];
  if (!hpke_build_suite_id(ctx, suite_id)) {
    return 0;
  }

  // psk_id_hash = LabeledExtract("", "psk_id_hash", psk_id)
  // TODO(davidben): Precompute this value and store it with the EVP_HPKE_KDF.
  const EVP_MD *hkdf_md = ctx->kdf->hkdf_md_func();
  uint8_t psk_id_hash[EVP_MAX_MD_SIZE];
  size_t psk_id_hash_len;
  if (!hpke_labeled_extract(hkdf_md, psk_id_hash, &psk_id_hash_len, NULL, 0,
                            suite_id, sizeof(suite_id), "psk_id_hash", NULL,
                            0)) {
    return 0;
  }

  // info_hash = LabeledExtract("", "info_hash", info)
  uint8_t info_hash[EVP_MAX_MD_SIZE];
  size_t info_hash_len;
  if (!hpke_labeled_extract(hkdf_md, info_hash, &info_hash_len, NULL, 0,
                            suite_id, sizeof(suite_id), "info_hash", info,
                            info_len)) {
    return 0;
  }

  // key_schedule_context = concat(mode, psk_id_hash, info_hash)
  uint8_t context[sizeof(uint8_t) + 2 * EVP_MAX_MD_SIZE];
  size_t context_len;
  CBB context_cbb;
  CBB_init_fixed(&context_cbb, context, sizeof(context));
  if (!CBB_add_u8(&context_cbb, mode) ||
      !CBB_add_bytes(&context_cbb, psk_id_hash, psk_id_hash_len) ||
      !CBB_add_bytes(&context_cbb, info_hash, info_hash_len) ||
      !CBB_finish(&context_cbb, NULL, &context_len)) {
    return 0;
  }

  // secret = LabeledExtract(shared_secret, "secret", psk)
  uint8_t secret[EVP_MAX_MD_SIZE];
  size_t secret_len;
  if (!hpke_labeled_extract(hkdf_md, secret, &secret_len, shared_secret,
                            shared_secret_len, suite_id, sizeof(suite_id),
                            "secret", NULL, 0)) {
    return 0;
  }

  // key = LabeledExpand(secret, "key", key_schedule_context, Nk)
  const EVP_AEAD *aead = EVP_HPKE_AEAD_aead(ctx->aead);
  uint8_t key[EVP_AEAD_MAX_KEY_LENGTH];
  const size_t kKeyLen = EVP_AEAD_key_length(aead);
  if (!hpke_labeled_expand(hkdf_md, key, kKeyLen, secret, secret_len, suite_id,
                           sizeof(suite_id), "key", context, context_len) ||
      !EVP_AEAD_CTX_init(&ctx->aead_ctx, aead, key, kKeyLen,
                         EVP_AEAD_DEFAULT_TAG_LENGTH, NULL)) {
    return 0;
  }

  // base_nonce = LabeledExpand(secret, "base_nonce", key_schedule_context, Nn)
  if (!hpke_labeled_expand(hkdf_md, ctx->base_nonce,
                           EVP_AEAD_nonce_length(aead), secret, secret_len,
                           suite_id, sizeof(suite_id), "base_nonce", context,
                           context_len)) {
    return 0;
  }

  // exporter_secret = LabeledExpand(secret, "exp", key_schedule_context, Nh)
  if (!hpke_labeled_expand(hkdf_md, ctx->exporter_secret, EVP_MD_size(hkdf_md),
                           secret, secret_len, suite_id, sizeof(suite_id),
                           "exp", context, context_len)) {
    return 0;
  }

  return 1;
}

void EVP_HPKE_CTX_zero(EVP_HPKE_CTX *ctx) {
  OPENSSL_memset(ctx, 0, sizeof(EVP_HPKE_CTX));
  EVP_AEAD_CTX_zero(&ctx->aead_ctx);
}

void EVP_HPKE_CTX_cleanup(EVP_HPKE_CTX *ctx) {
  EVP_AEAD_CTX_cleanup(&ctx->aead_ctx);
}

EVP_HPKE_CTX *EVP_HPKE_CTX_new(void) {
  EVP_HPKE_CTX *ctx = OPENSSL_zalloc(sizeof(EVP_HPKE_CTX));
  if (ctx == NULL) {
    return NULL;
  }
  // NO-OP: struct already zeroed
  //EVP_HPKE_CTX_zero(ctx);
  return ctx;
}

void EVP_HPKE_CTX_free(EVP_HPKE_CTX *ctx) {
  if (ctx != NULL) {
    EVP_HPKE_CTX_cleanup(ctx);
    OPENSSL_free(ctx);
  }
}

static int hpke_ctx_setup_sender(
    EVP_HPKE_CTX *ctx, uint8_t *out_enc, size_t *out_enc_len, size_t max_enc,
    const EVP_HPKE_KEM *kem, const EVP_HPKE_KDF *kdf, const EVP_HPKE_AEAD *aead,
    const uint8_t *peer_public_key, size_t peer_public_key_len,
    const uint8_t *info, size_t info_len) {
  uint8_t seed[MAX_SEED_LEN];
  AWSLC_ABORT_IF_NOT_ONE(RAND_bytes(seed, kem->seed_len));
  int ret = hpke_ctx_setup_sender_with_seed_for_testing(
      ctx, out_enc, out_enc_len, max_enc, kem, kdf, aead, peer_public_key,
      peer_public_key_len, info, info_len, seed, kem->seed_len);
  // The encapsulation entropy must not linger on the stack after setup.
  OPENSSL_cleanse(seed, sizeof(seed));
  return ret;
}

static int hpke_ctx_setup_sender_with_seed_for_testing(
    EVP_HPKE_CTX *ctx, uint8_t *out_enc, size_t *out_enc_len, size_t max_enc,
    const EVP_HPKE_KEM *kem, const EVP_HPKE_KDF *kdf, const EVP_HPKE_AEAD *aead,
    const uint8_t *peer_public_key, size_t peer_public_key_len,
    const uint8_t *info, size_t info_len, const uint8_t *seed,
    size_t seed_len) {
  EVP_HPKE_CTX_zero(ctx);
  ctx->is_sender = 1;
  ctx->kem = kem;
  ctx->kdf = kdf;
  ctx->aead = aead;
  uint8_t shared_secret[MAX_SHARED_SECRET_LEN];
  size_t shared_secret_len;
  int ret = kem->encap_with_seed(kem, shared_secret, &shared_secret_len,
                                 out_enc, out_enc_len, max_enc, peer_public_key,
                                 peer_public_key_len, seed, seed_len) &&
            hpke_key_schedule(ctx, HPKE_MODE_BASE, shared_secret,
                              shared_secret_len, info, info_len);
  // The shared secret is consumed by the key schedule and must not linger.
  OPENSSL_cleanse(shared_secret, sizeof(shared_secret));
  if (!ret) {
    EVP_HPKE_CTX_cleanup(ctx);
    return 0;
  }
  return 1;
}

static int hpke_ctx_setup_recipient(EVP_HPKE_CTX *ctx, const EVP_HPKE_KEY *key,
                                    const EVP_HPKE_KDF *kdf,
                                    const EVP_HPKE_AEAD *aead,
                                    const uint8_t *enc, size_t enc_len,
                                    const uint8_t *info, size_t info_len) {
  if (key->kem == NULL) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_NO_KEY_SET);
    return 0;
  }
  EVP_HPKE_CTX_zero(ctx);
  ctx->is_sender = 0;
  ctx->kem = key->kem;
  ctx->kdf = kdf;
  ctx->aead = aead;
  uint8_t shared_secret[MAX_SHARED_SECRET_LEN];
  size_t shared_secret_len;
  int ret = key->kem->decap(key, shared_secret, &shared_secret_len, enc,
                            enc_len) &&
            hpke_key_schedule(ctx, HPKE_MODE_BASE, shared_secret,
                              shared_secret_len, info, info_len);
  OPENSSL_cleanse(shared_secret, sizeof(shared_secret));
  if (!ret) {
    EVP_HPKE_CTX_cleanup(ctx);
    return 0;
  }
  return 1;
}


static int hpke_ctx_setup_auth_sender(
    EVP_HPKE_CTX *ctx, uint8_t *out_enc, size_t *out_enc_len, size_t max_enc,
    const EVP_HPKE_KEY *key, const EVP_HPKE_KDF *kdf, const EVP_HPKE_AEAD *aead,
    const uint8_t *peer_public_key, size_t peer_public_key_len,
    const uint8_t *info, size_t info_len) {
  if (key->kem == NULL) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_NO_KEY_SET);
    return 0;
  }
  // The callee re-checks this, for the benefit of direct callers, but fail
  // before drawing a seed for an operation which cannot succeed.
  if (key->kem->auth_encap_with_seed == NULL) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_OPERATION_NOT_SUPPORTED_FOR_THIS_KEYTYPE);
    return 0;
  }
  uint8_t seed[MAX_SEED_LEN];
  AWSLC_ABORT_IF_NOT_ONE(RAND_bytes(seed, key->kem->seed_len));
  int ret = hpke_ctx_setup_auth_sender_with_seed_for_testing(
      ctx, out_enc, out_enc_len, max_enc, key, kdf, aead, peer_public_key,
      peer_public_key_len, info, info_len, seed, key->kem->seed_len);
  // The encapsulation entropy must not linger on the stack after setup.
  OPENSSL_cleanse(seed, sizeof(seed));
  return ret;
}

static int hpke_ctx_setup_auth_sender_with_seed_for_testing(
    EVP_HPKE_CTX *ctx, uint8_t *out_enc, size_t *out_enc_len, size_t max_enc,
    const EVP_HPKE_KEY *key, const EVP_HPKE_KDF *kdf, const EVP_HPKE_AEAD *aead,
    const uint8_t *peer_public_key, size_t peer_public_key_len,
    const uint8_t *info, size_t info_len, const uint8_t *seed,
    size_t seed_len) {
  if (key->kem == NULL) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_NO_KEY_SET);
    return 0;
  }
  if (key->kem->auth_encap_with_seed == NULL) {
    // Not all HPKE KEMs support AuthEncap.
    OPENSSL_PUT_ERROR(EVP, EVP_R_OPERATION_NOT_SUPPORTED_FOR_THIS_KEYTYPE);
    return 0;
  }

  EVP_HPKE_CTX_zero(ctx);
  ctx->is_sender = 1;
  ctx->kem = key->kem;
  ctx->kdf = kdf;
  ctx->aead = aead;
  uint8_t shared_secret[MAX_SHARED_SECRET_LEN];
  size_t shared_secret_len;
  int ret = key->kem->auth_encap_with_seed(
                key, shared_secret, &shared_secret_len, out_enc, out_enc_len,
                max_enc, peer_public_key, peer_public_key_len, seed,
                seed_len) &&
            hpke_key_schedule(ctx, HPKE_MODE_AUTH, shared_secret,
                              shared_secret_len, info, info_len);
  OPENSSL_cleanse(shared_secret, sizeof(shared_secret));
  if (!ret) {
    EVP_HPKE_CTX_cleanup(ctx);
    return 0;
  }
  return 1;
}

static int hpke_ctx_setup_auth_recipient(
    EVP_HPKE_CTX *ctx, const EVP_HPKE_KEY *key, const EVP_HPKE_KDF *kdf,
    const EVP_HPKE_AEAD *aead, const uint8_t *enc, size_t enc_len,
    const uint8_t *info, size_t info_len, const uint8_t *peer_public_key,
    size_t peer_public_key_len) {
  if (key->kem == NULL) {
    OPENSSL_PUT_ERROR(EVP, EVP_R_NO_KEY_SET);
    return 0;
  }
  if (key->kem->auth_decap == NULL) {
    // Not all HPKE KEMs support AuthDecap.
    OPENSSL_PUT_ERROR(EVP, EVP_R_OPERATION_NOT_SUPPORTED_FOR_THIS_KEYTYPE);
    return 0;
  }

  EVP_HPKE_CTX_zero(ctx);
  ctx->is_sender = 0;
  ctx->kem = key->kem;
  ctx->kdf = kdf;
  ctx->aead = aead;
  uint8_t shared_secret[MAX_SHARED_SECRET_LEN];
  size_t shared_secret_len;
  int ret = key->kem->auth_decap(key, shared_secret, &shared_secret_len, enc,
                                 enc_len, peer_public_key,
                                 peer_public_key_len) &&
            hpke_key_schedule(ctx, HPKE_MODE_AUTH, shared_secret,
                              shared_secret_len, info, info_len);
  OPENSSL_cleanse(shared_secret, sizeof(shared_secret));
  if (!ret) {
    EVP_HPKE_CTX_cleanup(ctx);
    return 0;
  }
  return 1;
}

static void hpke_nonce(const EVP_HPKE_CTX *ctx, uint8_t *out_nonce,
                       size_t nonce_len) {
  assert(nonce_len >= 8);

  // Write padded big-endian bytes of |ctx->seq| to |out_nonce|.
  OPENSSL_memset(out_nonce, 0, nonce_len);
  uint64_t seq_copy = ctx->seq;
  for (size_t i = 0; i < 8; i++) {
    out_nonce[nonce_len - i - 1] = seq_copy & 0xff;
    seq_copy >>= 8;
  }

  // XOR the encoded sequence with the |ctx->base_nonce|.
  for (size_t i = 0; i < nonce_len; i++) {
    out_nonce[i] ^= ctx->base_nonce[i];
  }
}

static int hpke_ctx_open(EVP_HPKE_CTX *ctx, uint8_t *out, size_t *out_len,
                         size_t max_out_len, const uint8_t *in, size_t in_len,
                         const uint8_t *ad, size_t ad_len) {
  if (ctx->is_sender) {
    OPENSSL_PUT_ERROR(EVP, ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
    return 0;
  }
  if (ctx->seq == UINT64_MAX) {
    OPENSSL_PUT_ERROR(EVP, ERR_R_OVERFLOW);
    return 0;
  }

  uint8_t nonce[EVP_AEAD_MAX_NONCE_LENGTH];
  const size_t nonce_len = EVP_AEAD_nonce_length(ctx->aead_ctx.aead);
  hpke_nonce(ctx, nonce, nonce_len);

  if (!EVP_AEAD_CTX_open(&ctx->aead_ctx, out, out_len, max_out_len, nonce,
                         nonce_len, in, in_len, ad, ad_len)) {
    return 0;
  }
  ctx->seq++;
  return 1;
}

static int hpke_ctx_seal(EVP_HPKE_CTX *ctx, uint8_t *out, size_t *out_len,
                         size_t max_out_len, const uint8_t *in, size_t in_len,
                         const uint8_t *ad, size_t ad_len) {
  if (!ctx->is_sender) {
    OPENSSL_PUT_ERROR(EVP, ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
    return 0;
  }
  if (ctx->seq == UINT64_MAX) {
    OPENSSL_PUT_ERROR(EVP, ERR_R_OVERFLOW);
    return 0;
  }

  uint8_t nonce[EVP_AEAD_MAX_NONCE_LENGTH];
  const size_t nonce_len = EVP_AEAD_nonce_length(ctx->aead_ctx.aead);
  hpke_nonce(ctx, nonce, nonce_len);

  if (!EVP_AEAD_CTX_seal(&ctx->aead_ctx, out, out_len, max_out_len, nonce,
                         nonce_len, in, in_len, ad, ad_len)) {
    return 0;
  }
  ctx->seq++;
  return 1;
}

static int hpke_ctx_export(const EVP_HPKE_CTX *ctx, uint8_t *out,
                           size_t secret_len, const uint8_t *context,
                           size_t context_len) {
  uint8_t suite_id[HPKE_SUITE_ID_LEN];
  if (!hpke_build_suite_id(ctx, suite_id)) {
    return 0;
  }
  const EVP_MD *hkdf_md = ctx->kdf->hkdf_md_func();
  if (!hpke_labeled_expand(hkdf_md, out, secret_len, ctx->exporter_secret,
                           EVP_MD_size(hkdf_md), suite_id, sizeof(suite_id),
                           "sec", context, context_len)) {
    return 0;
  }
  return 1;
}

size_t EVP_HPKE_CTX_max_overhead(const EVP_HPKE_CTX *ctx) {
  assert(ctx->is_sender);
  return EVP_AEAD_max_overhead(EVP_AEAD_CTX_aead(&ctx->aead_ctx));
}

const EVP_HPKE_KEM *EVP_HPKE_CTX_kem(const EVP_HPKE_CTX *ctx) {
  return ctx->kem;
}

const EVP_HPKE_AEAD *EVP_HPKE_CTX_aead(const EVP_HPKE_CTX *ctx) {
  return ctx->aead;
}

const EVP_HPKE_KDF *EVP_HPKE_CTX_kdf(const EVP_HPKE_CTX *ctx) {
  return ctx->kdf;
}


// HPKE is not a FIPS-approved service: it appears in neither the service
// indicator design nor the FIPS approved-API list, and ACVP defines no HPKE
// algorithm to validate it against. The primitives it is built from are
// approved and do update the indicator -- HKDF, AES-GCM and |RAND_bytes| among
// them -- so each public entry point locks the indicator for the duration of
// the operation. Without this an HPKE call would leave a counter differential,
// which a caller would read as an approved service having been performed.
//
// The lock is a counter, so nesting is safe, and each wrapper unlocks on every
// path because it makes exactly one call.

int EVP_HPKE_KEY_init(EVP_HPKE_KEY *key, const EVP_HPKE_KEM *kem,
                      const uint8_t *priv_key, size_t priv_key_len) {
  FIPS_service_indicator_lock_state();
  int ret = hpke_key_init(key, kem, priv_key, priv_key_len);
  FIPS_service_indicator_unlock_state();
  return ret;
}

int EVP_HPKE_KEY_generate(EVP_HPKE_KEY *key, const EVP_HPKE_KEM *kem) {
  FIPS_service_indicator_lock_state();
  int ret = hpke_key_generate(key, kem);
  FIPS_service_indicator_unlock_state();
  return ret;
}

int EVP_HPKE_CTX_setup_sender_with_seed_for_testing(
    EVP_HPKE_CTX *ctx, uint8_t *out_enc, size_t *out_enc_len, size_t max_enc,
    const EVP_HPKE_KEM *kem, const EVP_HPKE_KDF *kdf, const EVP_HPKE_AEAD *aead,
    const uint8_t *peer_public_key, size_t peer_public_key_len,
    const uint8_t *info, size_t info_len, const uint8_t *seed,
    size_t seed_len) {
  FIPS_service_indicator_lock_state();
  int ret = hpke_ctx_setup_sender_with_seed_for_testing(
      ctx, out_enc, out_enc_len, max_enc, kem, kdf, aead, peer_public_key,
      peer_public_key_len, info, info_len, seed, seed_len);
  FIPS_service_indicator_unlock_state();
  return ret;
}

int EVP_HPKE_CTX_setup_sender(EVP_HPKE_CTX *ctx, uint8_t *out_enc,
                              size_t *out_enc_len, size_t max_enc,
                              const EVP_HPKE_KEM *kem, const EVP_HPKE_KDF *kdf,
                              const EVP_HPKE_AEAD *aead,
                              const uint8_t *peer_public_key,
                              size_t peer_public_key_len, const uint8_t *info,
                              size_t info_len) {
  FIPS_service_indicator_lock_state();
  int ret = hpke_ctx_setup_sender(ctx, out_enc, out_enc_len, max_enc, kem, kdf,
                                  aead, peer_public_key, peer_public_key_len,
                                  info, info_len);
  FIPS_service_indicator_unlock_state();
  return ret;
}

int EVP_HPKE_CTX_setup_auth_sender_with_seed_for_testing(
    EVP_HPKE_CTX *ctx, uint8_t *out_enc, size_t *out_enc_len, size_t max_enc,
    const EVP_HPKE_KEY *key, const EVP_HPKE_KDF *kdf, const EVP_HPKE_AEAD *aead,
    const uint8_t *peer_public_key, size_t peer_public_key_len,
    const uint8_t *info, size_t info_len, const uint8_t *seed,
    size_t seed_len) {
  FIPS_service_indicator_lock_state();
  int ret = hpke_ctx_setup_auth_sender_with_seed_for_testing(
      ctx, out_enc, out_enc_len, max_enc, key, kdf, aead, peer_public_key,
      peer_public_key_len, info, info_len, seed, seed_len);
  FIPS_service_indicator_unlock_state();
  return ret;
}

int EVP_HPKE_CTX_setup_auth_sender(
    EVP_HPKE_CTX *ctx, uint8_t *out_enc, size_t *out_enc_len, size_t max_enc,
    const EVP_HPKE_KEY *key, const EVP_HPKE_KDF *kdf, const EVP_HPKE_AEAD *aead,
    const uint8_t *peer_public_key, size_t peer_public_key_len,
    const uint8_t *info, size_t info_len) {
  FIPS_service_indicator_lock_state();
  int ret = hpke_ctx_setup_auth_sender(ctx, out_enc, out_enc_len, max_enc, key,
                                       kdf, aead, peer_public_key,
                                       peer_public_key_len, info, info_len);
  FIPS_service_indicator_unlock_state();
  return ret;
}

int EVP_HPKE_CTX_setup_recipient(EVP_HPKE_CTX *ctx, const EVP_HPKE_KEY *key,
                                 const EVP_HPKE_KDF *kdf,
                                 const EVP_HPKE_AEAD *aead, const uint8_t *enc,
                                 size_t enc_len, const uint8_t *info,
                                 size_t info_len) {
  FIPS_service_indicator_lock_state();
  int ret = hpke_ctx_setup_recipient(ctx, key, kdf, aead, enc, enc_len, info,
                                     info_len);
  FIPS_service_indicator_unlock_state();
  return ret;
}

int EVP_HPKE_CTX_setup_auth_recipient(
    EVP_HPKE_CTX *ctx, const EVP_HPKE_KEY *key, const EVP_HPKE_KDF *kdf,
    const EVP_HPKE_AEAD *aead, const uint8_t *enc, size_t enc_len,
    const uint8_t *info, size_t info_len, const uint8_t *peer_public_key,
    size_t peer_public_key_len) {
  FIPS_service_indicator_lock_state();
  int ret = hpke_ctx_setup_auth_recipient(ctx, key, kdf, aead, enc, enc_len,
                                          info, info_len, peer_public_key,
                                          peer_public_key_len);
  FIPS_service_indicator_unlock_state();
  return ret;
}

int EVP_HPKE_CTX_seal(EVP_HPKE_CTX *ctx, uint8_t *out, size_t *out_len,
                      size_t max_out_len, const uint8_t *in, size_t in_len,
                      const uint8_t *ad, size_t ad_len) {
  FIPS_service_indicator_lock_state();
  int ret =
      hpke_ctx_seal(ctx, out, out_len, max_out_len, in, in_len, ad, ad_len);
  FIPS_service_indicator_unlock_state();
  return ret;
}

int EVP_HPKE_CTX_open(EVP_HPKE_CTX *ctx, uint8_t *out, size_t *out_len,
                      size_t max_out_len, const uint8_t *in, size_t in_len,
                      const uint8_t *ad, size_t ad_len) {
  FIPS_service_indicator_lock_state();
  int ret =
      hpke_ctx_open(ctx, out, out_len, max_out_len, in, in_len, ad, ad_len);
  FIPS_service_indicator_unlock_state();
  return ret;
}

int EVP_HPKE_CTX_export(const EVP_HPKE_CTX *ctx, uint8_t *out,
                        size_t secret_len, const uint8_t *context,
                        size_t context_len) {
  FIPS_service_indicator_lock_state();
  int ret = hpke_ctx_export(ctx, out, secret_len, context, context_len);
  FIPS_service_indicator_unlock_state();
  return ret;
}
