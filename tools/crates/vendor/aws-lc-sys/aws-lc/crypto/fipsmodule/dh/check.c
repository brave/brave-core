// Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com) All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <openssl/dh.h>

#include <openssl/bn.h>
#include <openssl/err.h>

#include "internal.h"

int dh_check_params_fast(const DH *dh) {
  // Most operations scale with p and q.
  if (BN_is_negative(dh->p) || !BN_is_odd(dh->p) ||
      BN_num_bits(dh->p) > OPENSSL_DH_MAX_MODULUS_BITS) {
    OPENSSL_PUT_ERROR(DH, DH_R_INVALID_PARAMETERS);
    return 0;
  }

  // q must be bounded by p.
  if (dh->q != NULL && (BN_is_negative(dh->q) || BN_ucmp(dh->q, dh->p) > 0)) {
    OPENSSL_PUT_ERROR(DH, DH_R_INVALID_PARAMETERS);
    return 0;
  }

  // g must be an element of p's multiplicative group.
  if (BN_is_negative(dh->g) || BN_is_zero(dh->g) ||
      BN_ucmp(dh->g, dh->p) >= 0) {
    OPENSSL_PUT_ERROR(DH, DH_R_INVALID_PARAMETERS);
    return 0;
  }

  return 1;
}

int DH_check_pub_key(const DH *dh, const BIGNUM *pub_key, int *out_flags) {
  *out_flags = 0;
  if (!dh_check_params_fast(dh)) {
    return 0;
  }

  BN_CTX *ctx = BN_CTX_new();
  if (ctx == NULL) {
    return 0;
  }
  BN_CTX_start(ctx);

  int ok = 0;

  // Check |pub_key| is greater than 1.
  if (BN_cmp(pub_key, BN_value_one()) <= 0) {
    *out_flags |= DH_CHECK_PUBKEY_TOO_SMALL;
  }

  // Check |pub_key| is less than |dh->p| - 1.
  BIGNUM *tmp = BN_CTX_get(ctx);
  if (tmp == NULL ||
      !BN_copy(tmp, dh->p) ||
      !BN_sub_word(tmp, 1)) {
    goto err;
  }
  if (BN_cmp(pub_key, tmp) >= 0) {
    *out_flags |= DH_CHECK_PUBKEY_TOO_LARGE;
  }

  if (dh->q != NULL) {
    // Check |pub_key|^|dh->q| is 1 mod |dh->p|. This is necessary for RFC 5114
    // groups which are not safe primes but pick a generator on a prime-order
    // subgroup of size |dh->q|.
    if (!BN_mod_exp_mont(tmp, pub_key, dh->q, dh->p, ctx, NULL)) {
      goto err;
    }
    if (!BN_is_one(tmp)) {
      *out_flags |= DH_CHECK_PUBKEY_INVALID;
    }
  }

  ok = 1;

err:
  BN_CTX_end(ctx);
  BN_CTX_free(ctx);
  return ok;
}


// dh_is_known_safe_group returns one if |dh| is one of the well-known standard
// safe-prime groups (RFC 3526 MODP or RFC 7919 ffdhe), so that |DH_check| may
// accept it without primality testing, and zero otherwise.
//
// Both families are safe primes p = 2q+1 with g = 2, so recognizing p means
// both p and (p-1)/2 are prime by their construction as Sophie Germain primes.
// Every one of these primes is also 7 mod 8, so 2 is a quadratic residue and
// therefore generates the subgroup of order (p-1)/2. That is what lets
// |DH_check| skip both primality testing and the generator check.
static int dh_is_known_safe_group(const DH *dh) {
  // A different generator is not the named group, so let the full checks run.
  if (!BN_is_word(dh->g, 2)) {
    return 0;
  }

  // A subgroup order is optional. If supplied it must be the group's own, so
  // that |DH_check|'s q checks would all have passed; any other value falls
  // through to full validation.
  return dh_is_known_safe_prime_group(dh->p, dh->q);
}

// DH_check confirms that the Diffie-Hellman parameters dh are valid.
int DH_check(const DH *dh, int *out_flags) {
  *out_flags = 0;
  if (!dh_check_params_fast(dh)) {
    return 0;
  }

  // Keep this below |dh_check_params_fast()|, so that |DH_check| does not
  // depend on |dh_is_known_safe_group()| to bound the sizes and signs of p, q
  // and g. |dh_check_params_fast()| is only a few word comparisons anyway.
  if (dh_is_known_safe_group(dh)) {
    return 1;
  }

  // Check that p is a safe prime.
  int ok = 0, r, q_good = 0;
  BN_CTX *ctx = NULL;
  BIGNUM *t1 = NULL, *t2 = NULL;

  ctx = BN_CTX_new();
  if (ctx == NULL) {
    goto err;
  }
  BN_CTX_start(ctx);
  t1 = BN_CTX_get(ctx);
  if (t1 == NULL) {
    goto err;
  }
  t2 = BN_CTX_get(ctx);
  if (t2 == NULL) {
    goto err;
  }

  if (dh->q) {
    if (BN_ucmp(dh->p, dh->q) > 0) {
      q_good = 1;
    } else {
      *out_flags |= DH_CHECK_INVALID_Q_VALUE;
    }
  }

  if (q_good) {
    if (BN_cmp(dh->g, BN_value_one()) <= 0) {
      *out_flags |= DH_CHECK_NOT_SUITABLE_GENERATOR;
    } else if (BN_cmp(dh->g, dh->p) >= 0) {
      *out_flags |= DH_CHECK_NOT_SUITABLE_GENERATOR;
    } else {
      // Check g^q == 1 mod p
      if (!BN_mod_exp_mont(t1, dh->g, dh->q, dh->p, ctx, NULL)) {
        goto err;
      }
      if (!BN_is_one(t1)) {
        *out_flags |= DH_CHECK_NOT_SUITABLE_GENERATOR;
      }
    }
    r = BN_is_prime_ex(dh->q, BN_prime_checks_for_validation, ctx, NULL);
    if (r < 0) {
      goto err;
    }
    if (!r) {
      *out_flags |= DH_CHECK_Q_NOT_PRIME;
    }
    // Check p == 1 mod q  i.e. q divides p - 1
    if (!BN_div(t1, t2, dh->p, dh->q, ctx)) {
      goto err;
    }
    if (!BN_is_one(t2)) {
      *out_flags |= DH_CHECK_INVALID_Q_VALUE;
    }
  }

  r = BN_is_prime_ex(dh->p, BN_prime_checks_for_validation, ctx, NULL);
  if (r < 0) {
    goto err;
  }
  if (!r) {
    *out_flags |= DH_CHECK_P_NOT_PRIME;
  } else if (!dh->q) {
    if (!BN_rshift1(t1, dh->p)) {
      goto err;
    }
    r = BN_is_prime_ex(t1, BN_prime_checks_for_validation, ctx, NULL);
    if (r < 0) {
      goto err;
    }
    if (!r) {
      *out_flags |= DH_CHECK_P_NOT_SAFE_PRIME;
    }
  }
  ok = 1;

err:
  if (ctx != NULL) {
    BN_CTX_end(ctx);
    BN_CTX_free(ctx);
  }
  return ok;
}
