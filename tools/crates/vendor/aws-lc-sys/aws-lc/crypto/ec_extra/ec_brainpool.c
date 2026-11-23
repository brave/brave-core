// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

// Brainpool elliptic curve groups from RFC 5639.
// These curves are not FIPS-approved and are therefore defined outside
// the FIPS module boundary (crypto/fipsmodule/).

#include <openssl/ec.h>

#include <string.h>

#include <openssl/mem.h>
#include <openssl/nid.h>

#include "../fipsmodule/bn/internal.h"
#include "../fipsmodule/ec/internal.h"
#include "../internal.h"
#include "internal.h"

#include "../fipsmodule/ec/builtin_curves.h"

// DEFINE_METHOD_FUNCTION (from delocate.h) cannot be used outside the FIPS
// module because it relies on delocated BSS storage within bcm.o. Since
// Brainpool curves are not FIPS-approved, we use an equivalent lazy-init
// pattern directly.
#define DEFINE_CURVE_DATA(type, name)                                         \
  static type name##_storage;                                                 \
  static CRYPTO_once_t name##_once = CRYPTO_ONCE_INIT;                        \
  static void name##_do_init(type *out);                                      \
  static void name##_init(void) { name##_do_init(&name##_storage); }          \
  const type *name(void) {                                                    \
    CRYPTO_once(&name##_once, name##_init);                                   \
    return (const type *)&name##_storage;                                     \
  }                                                                           \
  static void name##_do_init(type *out)


// Duplicated from crypto/fipsmodule/ec/ec.c to avoid exposing
// FIPS-internal helpers across the module boundary.
static void ec_group_init_static_mont(BN_MONT_CTX *mont, size_t num_words,
                                      const BN_ULONG *modulus,
                                      const BN_ULONG *rr, uint64_t n0) {
  bn_set_static_words(&mont->N, modulus, num_words);
  bn_set_static_words(&mont->RR, rr, num_words);
#if defined(OPENSSL_64_BIT)
  mont->n0[0] = n0;
#elif defined(OPENSSL_32_BIT)
  mont->n0[0] = (uint32_t)n0;
  mont->n0[1] = (uint32_t)(n0 >> 32);
#else
#error "unknown word length"
#endif
}

// Brainpool curves have arbitrary a coefficients (not -3 like NIST curves,
// not 0 like secp256k1), so a is provided in precomputed Montgomery form.
static void ec_group_set_a_mont(EC_GROUP *group, const BN_ULONG *mont_a,
                                size_t num_bytes) {
  group->a_is_minus3 = 0;
  OPENSSL_memset(group->a.words, 0, sizeof(EC_FELEM));
  OPENSSL_memcpy(group->a.words, mont_a, num_bytes);
}

DEFINE_CURVE_DATA(EC_GROUP, EC_group_brainpoolP224r1) {
  out->curve_name = NID_brainpoolP224r1;
  out->comment = "brainpoolP224r1";
  // OID 1.3.36.3.3.2.8.1.1.5 — RFC 5639, Section 4.1
  static const uint8_t kOID[] = {0x2b, 0x24, 0x03, 0x03, 0x02, 0x08, 0x01,
                                 0x01, 0x05};
  OPENSSL_memcpy(out->oid, kOID, sizeof(kOID));
  out->oid_len = sizeof(kOID);

  ec_group_init_static_mont(
      &out->field, OPENSSL_ARRAY_SIZE(kbrainpoolP224r1Field),
      kbrainpoolP224r1Field, kbrainpoolP224r1FieldRR, kbrainpoolP224r1FieldN0);
  ec_group_init_static_mont(
      &out->order, OPENSSL_ARRAY_SIZE(kbrainpoolP224r1Order),
      kbrainpoolP224r1Order, kbrainpoolP224r1OrderRR, kbrainpoolP224r1OrderN0);

  out->meth = EC_GFp_mont_method();
  out->generator.group = out;
  OPENSSL_memcpy(out->generator.raw.X.words, kbrainpoolP224r1MontGX,
                 sizeof(kbrainpoolP224r1MontGX));
  OPENSSL_memcpy(out->generator.raw.Y.words, kbrainpoolP224r1MontGY,
                 sizeof(kbrainpoolP224r1MontGY));
  OPENSSL_memcpy(out->generator.raw.Z.words, kbrainpoolP224r1FieldR,
                 sizeof(kbrainpoolP224r1FieldR));
  OPENSSL_memcpy(out->b.words, kbrainpoolP224r1MontB,
                 sizeof(kbrainpoolP224r1MontB));

  ec_group_set_a_mont(out, kbrainpoolP224r1MontA,
                      sizeof(kbrainpoolP224r1MontA));
  out->has_order = 1;
  out->field_greater_than_order = 1;
  out->conv_form = POINT_CONVERSION_UNCOMPRESSED;
  out->mutable_ec_group = 0;
}

DEFINE_CURVE_DATA(EC_GROUP, EC_group_brainpoolP256r1) {
  out->curve_name = NID_brainpoolP256r1;
  out->comment = "brainpoolP256r1";
  // OID 1.3.36.3.3.2.8.1.1.7 — RFC 5639, Section 4.1
  static const uint8_t kOID[] = {0x2b, 0x24, 0x03, 0x03, 0x02, 0x08, 0x01,
                                 0x01, 0x07};
  OPENSSL_memcpy(out->oid, kOID, sizeof(kOID));
  out->oid_len = sizeof(kOID);

  ec_group_init_static_mont(
      &out->field, OPENSSL_ARRAY_SIZE(kbrainpoolP256r1Field),
      kbrainpoolP256r1Field, kbrainpoolP256r1FieldRR, kbrainpoolP256r1FieldN0);
  ec_group_init_static_mont(
      &out->order, OPENSSL_ARRAY_SIZE(kbrainpoolP256r1Order),
      kbrainpoolP256r1Order, kbrainpoolP256r1OrderRR, kbrainpoolP256r1OrderN0);

  out->meth = EC_GFp_mont_method();
  out->generator.group = out;
  OPENSSL_memcpy(out->generator.raw.X.words, kbrainpoolP256r1MontGX,
                 sizeof(kbrainpoolP256r1MontGX));
  OPENSSL_memcpy(out->generator.raw.Y.words, kbrainpoolP256r1MontGY,
                 sizeof(kbrainpoolP256r1MontGY));
  OPENSSL_memcpy(out->generator.raw.Z.words, kbrainpoolP256r1FieldR,
                 sizeof(kbrainpoolP256r1FieldR));
  OPENSSL_memcpy(out->b.words, kbrainpoolP256r1MontB,
                 sizeof(kbrainpoolP256r1MontB));

  ec_group_set_a_mont(out, kbrainpoolP256r1MontA,
                      sizeof(kbrainpoolP256r1MontA));
  out->has_order = 1;
  out->field_greater_than_order = 1;
  out->conv_form = POINT_CONVERSION_UNCOMPRESSED;
  out->mutable_ec_group = 0;
}

DEFINE_CURVE_DATA(EC_GROUP, EC_group_brainpoolP320r1) {
  out->curve_name = NID_brainpoolP320r1;
  out->comment = "brainpoolP320r1";
  // OID 1.3.36.3.3.2.8.1.1.9 — RFC 5639, Section 4.1
  static const uint8_t kOID[] = {0x2b, 0x24, 0x03, 0x03, 0x02, 0x08, 0x01,
                                 0x01, 0x09};
  OPENSSL_memcpy(out->oid, kOID, sizeof(kOID));
  out->oid_len = sizeof(kOID);

  ec_group_init_static_mont(
      &out->field, OPENSSL_ARRAY_SIZE(kbrainpoolP320r1Field),
      kbrainpoolP320r1Field, kbrainpoolP320r1FieldRR, kbrainpoolP320r1FieldN0);
  ec_group_init_static_mont(
      &out->order, OPENSSL_ARRAY_SIZE(kbrainpoolP320r1Order),
      kbrainpoolP320r1Order, kbrainpoolP320r1OrderRR, kbrainpoolP320r1OrderN0);

  out->meth = EC_GFp_mont_method();
  out->generator.group = out;
  OPENSSL_memcpy(out->generator.raw.X.words, kbrainpoolP320r1MontGX,
                 sizeof(kbrainpoolP320r1MontGX));
  OPENSSL_memcpy(out->generator.raw.Y.words, kbrainpoolP320r1MontGY,
                 sizeof(kbrainpoolP320r1MontGY));
  OPENSSL_memcpy(out->generator.raw.Z.words, kbrainpoolP320r1FieldR,
                 sizeof(kbrainpoolP320r1FieldR));
  OPENSSL_memcpy(out->b.words, kbrainpoolP320r1MontB,
                 sizeof(kbrainpoolP320r1MontB));

  ec_group_set_a_mont(out, kbrainpoolP320r1MontA,
                      sizeof(kbrainpoolP320r1MontA));
  out->has_order = 1;
  out->field_greater_than_order = 1;
  out->conv_form = POINT_CONVERSION_UNCOMPRESSED;
  out->mutable_ec_group = 0;
}

DEFINE_CURVE_DATA(EC_GROUP, EC_group_brainpoolP384r1) {
  out->curve_name = NID_brainpoolP384r1;
  out->comment = "brainpoolP384r1";
  // OID 1.3.36.3.3.2.8.1.1.11 — RFC 5639, Section 4.1
  static const uint8_t kOID[] = {0x2b, 0x24, 0x03, 0x03, 0x02, 0x08, 0x01,
                                 0x01, 0x0b};
  OPENSSL_memcpy(out->oid, kOID, sizeof(kOID));
  out->oid_len = sizeof(kOID);

  ec_group_init_static_mont(
      &out->field, OPENSSL_ARRAY_SIZE(kbrainpoolP384r1Field),
      kbrainpoolP384r1Field, kbrainpoolP384r1FieldRR, kbrainpoolP384r1FieldN0);
  ec_group_init_static_mont(
      &out->order, OPENSSL_ARRAY_SIZE(kbrainpoolP384r1Order),
      kbrainpoolP384r1Order, kbrainpoolP384r1OrderRR, kbrainpoolP384r1OrderN0);

  out->meth = EC_GFp_mont_method();
  out->generator.group = out;
  OPENSSL_memcpy(out->generator.raw.X.words, kbrainpoolP384r1MontGX,
                 sizeof(kbrainpoolP384r1MontGX));
  OPENSSL_memcpy(out->generator.raw.Y.words, kbrainpoolP384r1MontGY,
                 sizeof(kbrainpoolP384r1MontGY));
  OPENSSL_memcpy(out->generator.raw.Z.words, kbrainpoolP384r1FieldR,
                 sizeof(kbrainpoolP384r1FieldR));
  OPENSSL_memcpy(out->b.words, kbrainpoolP384r1MontB,
                 sizeof(kbrainpoolP384r1MontB));

  ec_group_set_a_mont(out, kbrainpoolP384r1MontA,
                      sizeof(kbrainpoolP384r1MontA));
  out->has_order = 1;
  out->field_greater_than_order = 1;
  out->conv_form = POINT_CONVERSION_UNCOMPRESSED;
  out->mutable_ec_group = 0;
}

DEFINE_CURVE_DATA(EC_GROUP, EC_group_brainpoolP512r1) {
  out->curve_name = NID_brainpoolP512r1;
  out->comment = "brainpoolP512r1";
  // OID 1.3.36.3.3.2.8.1.1.13 — RFC 5639, Section 4.1
  static const uint8_t kOID[] = {0x2b, 0x24, 0x03, 0x03, 0x02, 0x08, 0x01,
                                 0x01, 0x0d};
  OPENSSL_memcpy(out->oid, kOID, sizeof(kOID));
  out->oid_len = sizeof(kOID);

  ec_group_init_static_mont(
      &out->field, OPENSSL_ARRAY_SIZE(kbrainpoolP512r1Field),
      kbrainpoolP512r1Field, kbrainpoolP512r1FieldRR, kbrainpoolP512r1FieldN0);
  ec_group_init_static_mont(
      &out->order, OPENSSL_ARRAY_SIZE(kbrainpoolP512r1Order),
      kbrainpoolP512r1Order, kbrainpoolP512r1OrderRR, kbrainpoolP512r1OrderN0);

  out->meth = EC_GFp_mont_method();
  out->generator.group = out;
  OPENSSL_memcpy(out->generator.raw.X.words, kbrainpoolP512r1MontGX,
                 sizeof(kbrainpoolP512r1MontGX));
  OPENSSL_memcpy(out->generator.raw.Y.words, kbrainpoolP512r1MontGY,
                 sizeof(kbrainpoolP512r1MontGY));
  OPENSSL_memcpy(out->generator.raw.Z.words, kbrainpoolP512r1FieldR,
                 sizeof(kbrainpoolP512r1FieldR));
  OPENSSL_memcpy(out->b.words, kbrainpoolP512r1MontB,
                 sizeof(kbrainpoolP512r1MontB));

  ec_group_set_a_mont(out, kbrainpoolP512r1MontA,
                      sizeof(kbrainpoolP512r1MontA));
  out->has_order = 1;
  out->field_greater_than_order = 1;
  out->conv_form = POINT_CONVERSION_UNCOMPRESSED;
  out->mutable_ec_group = 0;
}

const EC_GROUP *ec_group_new_by_curve_name_nonfips(int nid) {
  switch (nid) {
    case NID_brainpoolP224r1:
      return EC_group_brainpoolP224r1();
    case NID_brainpoolP256r1:
      return EC_group_brainpoolP256r1();
    case NID_brainpoolP320r1:
      return EC_group_brainpoolP320r1();
    case NID_brainpoolP384r1:
      return EC_group_brainpoolP384r1();
    case NID_brainpoolP512r1:
      return EC_group_brainpoolP512r1();
    default:
      return NULL;
  }
}
