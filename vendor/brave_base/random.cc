/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/vendor/brave_base/random.h"

#include <bit>
#include <cmath>

#include "base/bits.h"
#include "crypto/random.h"

namespace brave_base::random {

uint64_t Uniform64() {
  uint64_t x;
  crypto::RandBytes(base::byte_span_from_ref(x));
  return x;
}

// Correct floating-point uniform [0,1] sampler which gives exactly
// the correct weight to every floating-point number in [0,1],
// i.e. the Lebesgue measure of the set of real numbers that is
// rounded to it.
//
// In principle this algorithm could return any floating-point number
// in [0,1], but in practice it cannot return 0 because the
// probability is 2^-1075; consequently you can reliably pass the
// result to, e.g., log, and be guaranteed to get a finite result.
//
// In contrast, if we simply divided a uniform random 53-bit or 64-bit
// integer by 2^53 or 2^64, the result would _not_ be guaranteed to be
// nonzero, _and_ it would exclude the result 1, which it should
// return with probability 2^-54.
double Uniform_01() {
  uint64_t e, x, u;

  // Draw an exponent with geometric distribution.
  e = 0;
  do {
    if ((x = Uniform64()) != 0)
      break;
    e += 64;
  } while (e < 1088);

  // Count the remaining leading zero bits to finish up the geometric
  // draw.
  //
  // If we stopped at e >= 1088, this means our RNG is broken.  In
  // that case, we could just as well abort the process.  But it is
  // also safe to call std::countl_zero at this point; it will
  // just return 64, and the exponent will be even more improbably
  // larger.
  e += std::countl_zero(x);

  u = Uniform64();

  return deterministic::Uniform_01(e, u);
}

// Nondeterministic distribution samplers.  These should call
// Uniform64 and Uniform_01 only, and pass them on to a deterministic
// transform in order to facilitate automatic testing.

double Exponential(double rate) {
  uint64_t s = Uniform64();
  double p0 = Uniform_01();
  return deterministic::Exponential(s, p0, rate);
}

uint64_t Geometric(double period) {
  uint64_t s = Uniform64();
  double p0 = Uniform_01();
  return deterministic::Geometric(s, p0, period);
}

namespace deterministic {

double StdExponential(uint64_t s, double p0) {
  // We want to evaluate log(p) for p near 0, and log1p(-p) for p near
  // 1.  We will decide which half of the interval we're lying in by a
  // coin toss, and then scale p0 appropriately.
  p0 *= 0.5;
  return ((s & 1) == 0) ? -log(p0) : -log1p(-p0);
}

double Exponential(uint64_t s, double p0, double rate) {
  return StdExponential(s, p0)/rate;
}

uint64_t Geometric(uint64_t s, double p0, double period) {
  return floor(StdExponential(s, p0)*period);
}

// If e has geometric distribution and u has uniform distribution,
// Uniform_01(e, u) has uniform distribution in [0, 1].
double Uniform_01(uint64_t e, uint64_t u) {
  double s;

  // Pick a normalized odd significand in (2^63, 2^64).  Choosing an
  // odd significand breaks ties, which occur with Lebesgue measure
  // zero in the reals but with nonzero probability in any finite
  // truncation of the binary expansion.
  u |= 0x8000000000000001ULL;

  // Round to double in [2^63, 2^64].
  s = static_cast<double>(u);

  // Scale into [1/2, 1].
  s *= ldexp(1, -64);

  // Apply the exponent.  This is a separate step, and done with
  // multiplication, because some platforms have broken ldexp.
  s *= ldexp(1, -e);

  return s;
}

}  // namespace deterministic

}  // namespace brave_base::random
