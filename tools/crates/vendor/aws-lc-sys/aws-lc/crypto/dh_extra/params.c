// Copyright (c) 2011 The OpenSSL Project.  All rights reserved.
// SPDX-License-Identifier: Apache-2.0
 
#include <openssl/dh.h>

#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/mem.h>

#include "../fipsmodule/bn/internal.h"
#include "../fipsmodule/dh/internal.h"


static BIGNUM *get_rfc3526_prime(BIGNUM *ret, unsigned bits) {
  BIGNUM *alloc = NULL;
  if (ret == NULL) {
    alloc = BN_new();
    if (alloc == NULL) {
      return NULL;
    }
    ret = alloc;
  }

  if (!dh_set_rfc3526_prime(ret, bits)) {
    BN_free(alloc);
    return NULL;
  }

  return ret;
}

BIGNUM *BN_get_rfc3526_prime_1536(BIGNUM *ret) {
  return get_rfc3526_prime(ret, 1536);
}

BIGNUM *BN_get_rfc3526_prime_2048(BIGNUM *ret) {
  return get_rfc3526_prime(ret, 2048);
}

BIGNUM *BN_get_rfc3526_prime_3072(BIGNUM *ret) {
  return get_rfc3526_prime(ret, 3072);
}

BIGNUM *BN_get_rfc3526_prime_4096(BIGNUM *ret) {
  return get_rfc3526_prime(ret, 4096);
}

BIGNUM *BN_get_rfc3526_prime_6144(BIGNUM *ret) {
  return get_rfc3526_prime(ret, 6144);
}

BIGNUM *BN_get_rfc3526_prime_8192(BIGNUM *ret) {
  return get_rfc3526_prime(ret, 8192);
}

int DH_generate_parameters_ex(DH *dh, int prime_bits, int generator,
                              BN_GENCB *cb) {
  // We generate DH parameters as follows
  // find a prime q which is prime_bits/2 bits long.
  // p=(2*q)+1 or (p-1)/2 = q
  // For this case, g is a generator if
  // g^((p-1)/q) mod p != 1 for values of q which are the factors of p-1.
  // Since the factors of p-1 are q and 2, we just need to check
  // g^2 mod p != 1 and g^q mod p != 1.
  //
  // Having said all that,
  // there is another special case method for the generators 2, 3 and 5.
  // for 2, p mod 24 == 11
  // for 3, p mod 12 == 5  <<<<< does not work for safe primes.
  // for 5, p mod 10 == 3 or 7
  //
  // Thanks to Phil Karn <karn@qualcomm.com> for the pointers about the
  // special generators and for answering some of my questions.
  //
  // I've implemented the second simple method :-).
  // Since DH should be using a safe prime (both p and q are prime),
  // this generator function can take a very very long time to run.

  // Actually there is no reason to insist that 'generator' be a generator.
  // It's just as OK (and in some sense better) to use a generator of the
  // order-q subgroup.

  if (prime_bits <= 0 || prime_bits > OPENSSL_DH_MAX_MODULUS_BITS) {
    OPENSSL_PUT_ERROR(DH, DH_R_MODULUS_TOO_LARGE);
    return 0;
  }

  BIGNUM *t1, *t2;
  int g, ok = 0;
  BN_CTX *ctx = NULL;

  ctx = BN_CTX_new();
  if (ctx == NULL) {
    goto err;
  }
  BN_CTX_start(ctx);
  t1 = BN_CTX_get(ctx);
  t2 = BN_CTX_get(ctx);
  if (t1 == NULL || t2 == NULL) {
    goto err;
  }

  // Make sure |dh| has the necessary elements
  if (dh->p == NULL) {
    dh->p = BN_new();
    if (dh->p == NULL) {
      goto err;
    }
  }
  if (dh->g == NULL) {
    dh->g = BN_new();
    if (dh->g == NULL) {
      goto err;
    }
  }

  if (generator <= 1) {
    OPENSSL_PUT_ERROR(DH, DH_R_BAD_GENERATOR);
    goto err;
  }
  if (generator == DH_GENERATOR_2) {
    if (!BN_set_word(t1, 24)) {
      goto err;
    }
    if (!BN_set_word(t2, 11)) {
      goto err;
    }
    g = 2;
  } else if (generator == DH_GENERATOR_5) {
    if (!BN_set_word(t1, 10)) {
      goto err;
    }
    if (!BN_set_word(t2, 3)) {
      goto err;
    }
    // BN_set_word(t3,7); just have to miss
    // out on these ones :-(
    g = 5;
  } else {
    // in the general case, don't worry if 'generator' is a
    // generator or not: since we are using safe primes,
    // it will generate either an order-q or an order-2q group,
    // which both is OK
    if (!BN_set_word(t1, 2)) {
      goto err;
    }
    if (!BN_set_word(t2, 1)) {
      goto err;
    }
    g = generator;
  }

  if (!BN_generate_prime_ex(dh->p, prime_bits, 1, t1, t2, cb)) {
    goto err;
  }
  if (!BN_GENCB_call(cb, 3, 0)) {
    goto err;
  }
  if (!BN_set_word(dh->g, g)) {
    goto err;
  }
  ok = 1;

err:
  if (!ok) {
    OPENSSL_PUT_ERROR(DH, ERR_R_BN_LIB);
  }

  if (ctx != NULL) {
    BN_CTX_end(ctx);
    BN_CTX_free(ctx);
  }
  return ok;
}

static int int_dh_bn_cpy(BIGNUM **dst, const BIGNUM *src) {
  BIGNUM *a = NULL;

  if (src) {
    a = BN_dup(src);
    if (!a) {
      return 0;
    }
  }

  BN_free(*dst);
  *dst = a;
  return 1;
}

static int int_dh_param_copy(DH *to, const DH *from, int is_x942) {
  if (is_x942 == -1) {
    is_x942 = !!from->q;
  }
  if (!int_dh_bn_cpy(&to->p, from->p) ||
      !int_dh_bn_cpy(&to->g, from->g)) {
    return 0;
  }

  if (!is_x942) {
    return 1;
  }

  if (!int_dh_bn_cpy(&to->q, from->q)) {
    return 0;
  }

  return 1;
}

DH *DHparams_dup(const DH *dh) {
  DH *ret = DH_new();
  if (!ret) {
    return NULL;
  }

  if (!int_dh_param_copy(ret, dh, -1)) {
    DH_free(ret);
    return NULL;
  }

  return ret;
}

DH *DH_get_2048_256(void) { 
  static const BN_ULONG dh2048_256_p[] = {
    TOBN(0xDB094AE9, 0x1E1A1597), TOBN(0x693877FA, 0xD7EF09CA),
    TOBN(0x6116D227, 0x6E11715F), TOBN(0xA4B54330, 0xC198AF12),
    TOBN(0x75F26375, 0xD7014103), TOBN(0xC3A3960A, 0x54E710C3),
    TOBN(0xDED4010A, 0xBD0BE621), TOBN(0xC0B857F6, 0x89962856),
    TOBN(0xB3CA3F79, 0x71506026), TOBN(0x1CCACB83, 0xE6B486F6),
    TOBN(0x67E144E5, 0x14056425), TOBN(0xF6A167B5, 0xA41825D9),
    TOBN(0x3AD83477, 0x96524D8E), TOBN(0xF13C6D9A, 0x51BFA4AB),
    TOBN(0x2D525267, 0x35488A0E), TOBN(0xB63ACAE1, 0xCAA6B790),
    TOBN(0x4FDB70C5, 0x81B23F76), TOBN(0xBC39A0BF, 0x12307F5C),
    TOBN(0xB941F54E, 0xB1E59BB8), TOBN(0x6C5BFC11, 0xD45F9088),
    TOBN(0x22E0B1EF, 0x4275BF7B), TOBN(0x91F9E672, 0x5B4758C0),
    TOBN(0x5A8A9D30, 0x6BCF67ED), TOBN(0x209E0C64, 0x97517ABD),
    TOBN(0x3BF4296D, 0x830E9A7C), TOBN(0x16C3D911, 0x34096FAA),
    TOBN(0xFAF7DF45, 0x61B2AA30), TOBN(0xE00DF8F1, 0xD61957D4),
    TOBN(0x5D2CEED4, 0x435E3B00), TOBN(0x8CEEF608, 0x660DD0F2),
    TOBN(0xFFBBD19C, 0x65195999), TOBN(0x87A8E61D, 0xB4B6663C),
  };
  static const BN_ULONG dh2048_256_g[] = {
      TOBN(0x664B4C0F, 0x6CC41659), TOBN(0x5E2327CF, 0xEF98C582),
      TOBN(0xD647D148, 0xD4795451), TOBN(0x2F630784, 0x90F00EF8),
      TOBN(0x184B523D, 0x1DB246C3), TOBN(0xC7891428, 0xCDC67EB6),
      TOBN(0x7FD02837, 0x0DF92B52), TOBN(0xB3353BBB, 0x64E0EC37),
      TOBN(0xECD06E15, 0x57CD0915), TOBN(0xB7D2BBD2, 0xDF016199),
      TOBN(0xC8484B1E, 0x052588B9), TOBN(0xDB2A3B73, 0x13D3FE14),
      TOBN(0xD052B985, 0xD182EA0A), TOBN(0xA4BD1BFF, 0xE83B9C80),
      TOBN(0xDFC967C1, 0xFB3F2E55), TOBN(0xB5045AF2, 0x767164E1),
      TOBN(0x1D14348F, 0x6F2F9193), TOBN(0x64E67982, 0x428EBC83),
      TOBN(0x8AC376D2, 0x82D6ED38), TOBN(0x777DE62A, 0xAAB8A862),
      TOBN(0xDDF463E5, 0xE9EC144B), TOBN(0x0196F931, 0xC77A57F2),
      TOBN(0xA55AE313, 0x41000A65), TOBN(0x901228F8, 0xC28CBB18),
      TOBN(0xBC3773BF, 0x7E8C6F62), TOBN(0xBE3A6C1B, 0x0C6B47B1),
      TOBN(0xFF4FED4A, 0xAC0BB555), TOBN(0x10DBC150, 0x77BE463F),
      TOBN(0x07F4793A, 0x1A0BA125), TOBN(0x4CA7B18F, 0x21EF2054),
      TOBN(0x2E775066, 0x60EDBD48), TOBN(0x3FB32C9B, 0x73134D0B),
  };
  static const BN_ULONG dh2048_256_q[] = {
      TOBN(0xA308B0FE, 0x64F5FBD3), TOBN(0x99B1A47D, 0x1EB3750B),
      TOBN(0xB4479976, 0x40129DA2), TOBN(0x8CF83642, 0xA709A097),
  };

  struct standard_parameters {
    BIGNUM p, q, g;
  };

  static const struct standard_parameters dh2048_256 = {
    STATIC_BIGNUM(dh2048_256_p),
    STATIC_BIGNUM(dh2048_256_q),
    STATIC_BIGNUM(dh2048_256_g),
  };

  DH *dh = DH_new();
  if (!dh) {
    return NULL;
  }

  dh->p = BN_dup(&dh2048_256.p);
  dh->q = BN_dup(&dh2048_256.q);
  dh->g = BN_dup(&dh2048_256.g);
  if (!dh->p || !dh->q || !dh->g) {
    DH_free(dh);
    return NULL;
  }

  return dh;
}
