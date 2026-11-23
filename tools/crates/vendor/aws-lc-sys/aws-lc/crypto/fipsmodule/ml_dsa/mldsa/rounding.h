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

#ifndef MLD_ROUNDING_H
#define MLD_ROUNDING_H

#include "cbmc.h"
#include "common.h"
#include "ct.h"
#include "debug.h"

/* Parameter set namespacing
 * This is to facilitate building multiple instances
 * of mldsa-native (e.g. with varying parameter sets)
 * within a single compilation unit. */
#define mld_power2round MLD_ADD_PARAM_SET(mld_power2round)
#define mld_decompose MLD_ADD_PARAM_SET(mld_decompose)
#define mld_make_hint MLD_ADD_PARAM_SET(mld_make_hint)
#define mld_use_hint MLD_ADD_PARAM_SET(mld_use_hint)
/* End of parameter set namespacing */

#define MLD_2_POW_D (1 << MLDSA_D)

/**
 * For finite field element a, compute a0, a1 such that
 * a mod^+ MLDSA_Q = a1*2^MLDSA_D + a0 with
 * -2^{MLDSA_D-1} < a0 <= 2^{MLDSA_D-1}. Assumes a to be standard
 * representative.
 *
 * @spec{Implements @[FIPS204, Algorithm 35, Power2Round].}
 *
 * @reference{In the reference implementation, a1 is passed as a return value
 * instead.}
 *
 * @param[out] a0 Pointer to output element a0.
 * @param[out] a1 Pointer to output element a1.
 * @param      a  Input element.
 */
static MLD_INLINE void mld_power2round(int32_t *a0, int32_t *a1, int32_t a)
__contract__(
  requires(memory_no_alias(a0, sizeof(int32_t)))
  requires(memory_no_alias(a1, sizeof(int32_t)))
  requires(a >= 0 && a < MLDSA_Q)
  assigns(memory_slice(a0, sizeof(int32_t)))
  assigns(memory_slice(a1, sizeof(int32_t)))
  ensures(*a0 > -(MLD_2_POW_D/2) && *a0 <= (MLD_2_POW_D/2))
  ensures(*a1 >= 0 && *a1 <= (MLDSA_Q - 1) / MLD_2_POW_D)
  ensures((*a1 * MLD_2_POW_D + *a0 - a) % MLDSA_Q == 0)
)
{
  *a1 = (a + (1 << (MLDSA_D - 1)) - 1) >> MLDSA_D;
  *a0 = a - (*a1 << MLDSA_D);
}

/**
 * For finite field element a, compute high and low bits a0, a1 such that
 * a mod^+ MLDSA_Q = a1 * 2 * MLDSA_GAMMA2 + a0 with
 * -MLDSA_GAMMA2 < a0 <= MLDSA_GAMMA2 except if
 * a1 = (MLDSA_Q-1)/(MLDSA_GAMMA2*2) where we set a1 = 0 and
 * -MLDSA_GAMMA2 <= a0 = a mod^+ MLDSA_Q - MLDSA_Q < 0. Assumes a to be
 * standard representative.
 *
 * @spec{Implements @[FIPS204, Algorithm 36, Decompose].}
 *
 * @reference{In the reference implementation, a1 is passed as a return value
 * instead.}
 *
 * @param[out] a0 Pointer to output element a0.
 * @param[out] a1 Pointer to output element a1.
 * @param      a  Input element.
 */
static MLD_INLINE void mld_decompose(int32_t *a0, int32_t *a1, int32_t a)
__contract__(
  requires(memory_no_alias(a0, sizeof(int32_t)))
  requires(memory_no_alias(a1, sizeof(int32_t)))
  requires(a >= 0 && a < MLDSA_Q)
  assigns(memory_slice(a0, sizeof(int32_t)))
  assigns(memory_slice(a1, sizeof(int32_t)))
  /* a0 = -MLDSA_GAMMA2 occurs exactly when a = MLDSA_Q - MLDSA_GAMMA2: the
   * border case of Decompose where a1 = (MLDSA_Q-1)/(2*MLDSA_GAMMA2) is
   * wrapped to 0 and a0 = a - MLDSA_Q (@[FIPS204, Algorithm 36, Decompose]) */
  ensures(*a0 >= -MLDSA_GAMMA2  && *a0 <= MLDSA_GAMMA2)
  ensures(*a1 >= 0 && *a1 < (MLDSA_Q-1)/(2*MLDSA_GAMMA2))
  ensures((*a1 * 2 * MLDSA_GAMMA2 + *a0 - a) % MLDSA_Q == 0)
)
{
  /*
   * The goal is to compute f1 = round-(f / (2*GAMMA2)), which can be computed
   * alternatively as round-(f / (128B)) = round-(ceil(f / 128) / B) where
   * B = 2*GAMMA2 / 128. Here round-() denotes "round half down".
   *
   * The equality round-(f / (128B)) = round-(ceil(f / 128) / B) can deduced
   * as follows. Since changing f to align-up(f, 128) can move f onto but not
   * across a rounding boundary for division by 128*B (note that we need B to be
   * even for this to work), and round- rounds down on the boundary, we have
   *
   *   round-(f / (128B)) = round-(align-up(f, 128) / (128B))
   *                      = round-((align-up(f, 128) / 128) / B)
   *                      = round-(ceil(f / 128) / B).
   */
  *a1 = (a + 127) >> 7;
  /* We know a >= 0 and a < MLDSA_Q, so... */
  /* check-magic: 65472 == round((MLDSA_Q-1)/128) */
  mld_assert(*a1 >= 0 && *a1 <= 65472);

#if MLD_CONFIG_PARAMETER_SET == 44
  /* check-magic: 1488 == 2 * intdiv(intdiv(MLDSA_Q - 1, 88), 128) */
  /* check-magic: 11275 == floor(2**24 / 1488) */
  /* check-magic: 1560281088 == 1 / (1 / 1488 - 11275 / 2**24) */
  /*
   * Compute f1 = round-(f1' / B) ≈ round(f1' * 11275 / 2^24). This is exact for
   * 0 <= f1' < 2^16.
   *
   * To see this, consider the (signed) error f1' * (1 / B - 11275 / 2^24)
   * between f1' / B and the (under-)approximation f1' * 11275 / 2^24. Because
   * eps := 1 / B - 11275 / 2^24 is 1 / 1560281088 ≈ 2^(-30.54) < 2^(-30), we
   * have 0 <= f1' * eps < 2^16 * 2^(-30) = 1 / 2^14 < 1 / 2^11 < 1 / B (note
   * that f1' is non-negative).
   *
   * On the other hand, 1 / B is the spacing between the integral multiples
   * of 1 / B, which includes all rounding boundaries n + 0.5 (since B is even).
   * Hence, if f1' / B is not of the form n + 0.5, then it is at least 1 / B
   * away from the nearest rounding boundary, so moving from f1' / B to
   * f1' * 11275 / 2^24 does not affect the rounding result, no matter the type
   * of rounding used in either side. In particular, we have round-(f1' / B) =
   * round(f1' * 11275 / 2^24) as claimed.
   *
   * As for the remaining case where f1' / B _is_ of the form n + 0.5, because
   * f1' * 11275 / 2^24 is slightly but strictly below f1' / B = n + 0.5 (note
   * that f1' and thus the error f1' * eps cannot be 0 here), it is always
   * rounded down to n. More precisely, we have round-(f1' / B) =
   * round(f1' * 11275 / 2^24), where the round-down on the LHS is essential,
   * and on the RHS the type of rounding again does not matter. This concludes
   * the proof.
   *
   * See proofs/isabelle/compress for a formalization of the above argument.
   */
  *a1 = (*a1 * 11275 + ((int32_t)1 << 23)) >> 24;
  mld_assert(*a1 >= 0 && *a1 <= 44);

  *a1 = mld_ct_sel_int32(0, *a1, mld_ct_cmask_neg_i32(43 - *a1));
  mld_assert(*a1 >= 0 && *a1 <= 43);
#else /* MLD_CONFIG_PARAMETER_SET == 44 */
  /* check-magic: 4092 == 2 * intdiv(intdiv(MLDSA_Q - 1, 32), 128) */
  /* check-magic: 1025 == floor(2**22 / 4092) */
  /* check-magic: 4290772992 == 1 / (1 / 4092 - 1025 / 2**22) */
  /*
   * Compute f1 = round-(f1' / B) ≈ round(f1' * 1025 / 2^22). This is exact for
   * 0 <= f1' < 2^16. Following the same argument above, it suffices to show
   * that f1' * eps < 1 / B, where eps := 1 / B - 1025 / 2^22. Indeed, we have
   * eps = 1 / 4290772992 ≈ 2^(-31.99) < 2^(-31), therefore f1' * eps <
   * 2^16 * 2^(-31) = 1 / 2^15 < 1 / 2^12 < 1 / B.
   */
  *a1 = (*a1 * 1025 + ((int32_t)1 << 21)) >> 22;
  mld_assert(*a1 >= 0 && *a1 <= 16);

  *a1 &= 15;
  mld_assert(*a1 >= 0 && *a1 <= 15);

#endif /* MLD_CONFIG_PARAMETER_SET != 44 */

  *a0 = a - *a1 * 2 * MLDSA_GAMMA2;
  *a0 = mld_ct_sel_int32(*a0 - MLDSA_Q, *a0,
                         mld_ct_cmask_neg_i32((MLDSA_Q - 1) / 2 - *a0));
}

/**
 * Decide a single hint bit from the low part a0 and high part a1 of a
 * coefficient: return 1 unless a0 lies in the range (-GAMMA2, GAMMA2] that
 * LowBits would produce, with the boundary value -GAMMA2 also admitted when
 * a1 == 0 (the Decompose border case).
 *
 * @note This is not a line-for-line implementation of FIPS 204's MakeHint(z, r)
 * (@[FIPS204, Algorithm 39, MakeHint]), which takes two ring elements and
 * returns [[HighBits(r) != HighBits(r + z)]]. Instead, it takes the already
 * decomposed low/high parts (a0, a1) of a coefficient and decides the hint bit
 * from them directly. As explained in the block comment of
 * mld_attempt_signature_generation (sign.c), for the specific values that arise
 * during signing -- a0 = w0 - cs2 + ct0 and a1 = w1 = HighBits(w) -- this is
 * equivalent to the spec's MakeHint(-ct0, w - cs2 + ct0) coefficient-wise.
 * Because it consumes (a0, a1) rather than (z, r), it relies on the caller
 * having computed a compatible decomposition.
 *
 * @param a0 Low bits of input element.
 * @param a1 High bits of input element.
 *
 * @return 1 if overflow, 0 otherwise.
 */
MLD_MUST_CHECK_RETURN_VALUE
static MLD_INLINE unsigned int mld_make_hint(int32_t a0, int32_t a1)
__contract__(
  ensures(return_value >= 0 && return_value <= 1)
  ensures(return_value == (a0 > MLDSA_GAMMA2 || a0 < -MLDSA_GAMMA2 ||
                           (a0 == -MLDSA_GAMMA2 && a1 != 0)))
)
{
  if (a0 > MLDSA_GAMMA2 || a0 < -MLDSA_GAMMA2 ||
      (a0 == -MLDSA_GAMMA2 && a1 != 0))
  {
    return 1;
  }

  return 0;
}

/**
 * Correct high bits according to hint.
 *
 * @spec{Implements @[FIPS204, Algorithm 40, UseHint].}
 *
 * @param a    Input element.
 * @param hint Hint bit.
 *
 * @return Corrected high bits.
 */
MLD_MUST_CHECK_RETURN_VALUE
static MLD_INLINE int32_t mld_use_hint(int32_t a, int32_t hint)
__contract__(
  requires(hint >= 0 && hint <= 1)
  requires(a >= 0 && a < MLDSA_Q)
  ensures(return_value >= 0 && return_value < (MLDSA_Q-1)/(2*MLDSA_GAMMA2))
)
{
  int32_t a0, a1;

  mld_decompose(&a0, &a1, a);
  if (hint == 0)
  {
    return a1;
  }

#if MLD_CONFIG_PARAMETER_SET == 44
  if (a0 > 0)
  {
    return (a1 == 43) ? 0 : a1 + 1;
  }
  else
  {
    return (a1 == 0) ? 43 : a1 - 1;
  }
#else  /* MLD_CONFIG_PARAMETER_SET == 44 */
  if (a0 > 0)
  {
    return (a1 + 1) & 15;
  }
  else
  {
    return (a1 - 1) & 15;
  }
#endif /* MLD_CONFIG_PARAMETER_SET != 44 */
}


#endif /* !MLD_ROUNDING_H */
