/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define _USE_MATH_DEFINES       // Microsoft, please define M_LN2 in <cmath>

#include <cmath>

#include "brave_base/random.h"
#include "testing/gtest/include/gtest/gtest.h"

class BraveRandomDeterministicTest : public testing::Test {
 public:
  BraveRandomDeterministicTest() {}
  BraveRandomDeterministicTest(const BraveRandomDeterministicTest&) = delete;
  BraveRandomDeterministicTest& operator=(const BraveRandomDeterministicTest&) =
      delete;
  ~BraveRandomDeterministicTest() override {}

 private:
};

TEST_F(BraveRandomDeterministicTest, Uniform_01) {
  using brave_base::random::deterministic::Uniform_01;

  // We use EXPECT_EQ here, _not_ EXPECT_DOUBLE_EQ, because these
  // tests are for equality, not for low relative error.

  // Zero can be returned, in principle, but flipping 1088 tails
  // before the first heads is not going to happen.  Significand makes
  // no difference with this much exponent.
  EXPECT_EQ(0, Uniform_01(1088, 0x7b35e177a2288418ULL));

  // Straddle the boundary of zero and subnormals.  Subnormals can be
  // returned, though again only improbably.
  EXPECT_EQ(0, Uniform_01(1074, 0x0000000000000000ULL));
  EXPECT_EQ(0, Uniform_01(1074, 0x8000000000000000ULL));
  EXPECT_EQ(0, Uniform_01(1074, 0x0000000000000200ULL));
  EXPECT_EQ(0, Uniform_01(1074, 0x8000000000000200ULL));
  EXPECT_EQ(0, Uniform_01(1074, 0x00000000000003ffULL));
  EXPECT_EQ(0, Uniform_01(1074, 0x80000000000003ffULL));
  EXPECT_EQ(4.9406564584124654e-324, Uniform_01(1074, 0x0000000000000400ULL));
  EXPECT_EQ(4.9406564584124654e-324, Uniform_01(1074, 0x8000000000000400ULL));
  EXPECT_EQ(4.9406564584124654e-324, Uniform_01(1074, 0x7fffffffffffffffULL));
  EXPECT_EQ(4.9406564584124654e-324, Uniform_01(1074, 0xffffffffffffffffULL));

  // Move up an exponent, on to the next subnormal.
  EXPECT_EQ(4.9406564584124654e-324, Uniform_01(1073, 0x0000000000000000ULL));
  EXPECT_EQ(4.9406564584124654e-324, Uniform_01(1073, 0x0000000000000001ULL));
  EXPECT_EQ(4.9406564584124654e-324, Uniform_01(1073, 0x1fffffffffffffffULL));
  EXPECT_EQ(4.9406564584124654e-324, Uniform_01(1073, 0x9fffffffffffffffULL));
  EXPECT_EQ(4.9406564584124654e-324, Uniform_01(1073, 0x2fffffffffffffffULL));
  EXPECT_EQ(4.9406564584124654e-324, Uniform_01(1073, 0xafffffffffffffffULL));
  EXPECT_EQ(9.8813129168249309e-324, Uniform_01(1073, 0x3fffffffffffffffULL));
  EXPECT_EQ(9.8813129168249309e-324, Uniform_01(1073, 0xbfffffffffffffffULL));

  // Straddle the boundary of normals and subnormals.
  EXPECT_EQ(2.2250738585072009e-308, Uniform_01(1022, 0x7ffffffffffff3ffULL));
  EXPECT_EQ(2.2250738585072014e-308, Uniform_01(1022, 0x7ffffffffffff400ULL));

  // Check a few normal cases with different exponents and significands.
  EXPECT_EQ(0.12500, Uniform_01(2, 0x0000000000000000ULL));
  EXPECT_EQ(0.15625, Uniform_01(2, 0x2000000000000000ULL));
  EXPECT_EQ(0.18750, Uniform_01(2, 0x4000000000000000ULL));
  EXPECT_EQ(0.21875, Uniform_01(2, 0x6000000000000000ULL));
  EXPECT_EQ(0.25000, Uniform_01(1, 0x0000000000000000ULL));
  EXPECT_EQ(0.31250, Uniform_01(1, 0x2000000000000000ULL));
  EXPECT_EQ(0.37500, Uniform_01(1, 0x4000000000000000ULL));
  EXPECT_EQ(0.43750, Uniform_01(1, 0x6000000000000000ULL));
  EXPECT_EQ(0.50000, Uniform_01(0, 0x0000000000000000ULL));
  EXPECT_EQ(0.62500, Uniform_01(0, 0x2000000000000000ULL));
  EXPECT_EQ(0.75000, Uniform_01(0, 0x4000000000000000ULL));
  EXPECT_EQ(0.87500, Uniform_01(0, 0x6000000000000000ULL));

  // Straddle the boundary of <1 and 1.
  EXPECT_EQ(0.99999999999999978, Uniform_01(0, 0xfffffffffffff200ULL));
  EXPECT_EQ(0.99999999999999978, Uniform_01(0, 0xfffffffffffff3ffULL));
  EXPECT_EQ(0.99999999999999989, Uniform_01(0, 0xfffffffffffff400ULL));
  EXPECT_EQ(0.99999999999999989, Uniform_01(0, 0xfffffffffffff800ULL));
  EXPECT_EQ(0.99999999999999989, Uniform_01(0, 0xfffffffffffffbffULL));
  EXPECT_EQ(1.00000000000000000, Uniform_01(0, 0xfffffffffffffc00ULL));
  EXPECT_EQ(1.00000000000000000, Uniform_01(0, 0xffffffffffffffffULL));
}

TEST_F(BraveRandomDeterministicTest, StdExponential) {
  using brave_base::random::deterministic::StdExponential;

  // Check edge cases.  Only for the smallest subnormal do we get
  // +inf or 0; these have negligible probability.
  EXPECT_DOUBLE_EQ(HUGE_VAL, StdExponential(0, 4.9406564584124654e-324));
  EXPECT_DOUBLE_EQ(744.4400719213812,
                   StdExponential(0, 9.8813129168249309e-324));
  EXPECT_DOUBLE_EQ(3*M_LN2, StdExponential(0, 0.25));
  EXPECT_DOUBLE_EQ(2*M_LN2, StdExponential(0, 0.5));
  EXPECT_DOUBLE_EQ(M_LN2, StdExponential(0, 1));
  EXPECT_DOUBLE_EQ(M_LN2, StdExponential(1, 1));
  EXPECT_DOUBLE_EQ(-log(0.75), StdExponential(1, 0.5));
  EXPECT_DOUBLE_EQ(-log(0.875), StdExponential(1, 0.25));
  EXPECT_DOUBLE_EQ(4.9406564584124654e-324,
                   StdExponential(1, 9.8813129168249309e-324));
  EXPECT_DOUBLE_EQ(0, StdExponential(1, 4.9406564584124654e-324));
}

TEST_F(BraveRandomDeterministicTest, Exponential) {
  using brave_base::random::deterministic::Exponential;

  // Check a rate below 1, i.e. a scale above 1.
  //
  // (-2*log(smallest subnormal) turns out to be about 1488.  Sorry!
  // No Nazi numerology intended here.)
  EXPECT_DOUBLE_EQ(HUGE_VAL, Exponential(0, 4.9406564584124654e-324, 0.5));
  EXPECT_DOUBLE_EQ(1488.8801438427624,
                   Exponential(0, 9.8813129168249309e-324, 0.5));
  EXPECT_DOUBLE_EQ(6*M_LN2, Exponential(0, 0.25, 0.5));
  EXPECT_DOUBLE_EQ(4*M_LN2, Exponential(0, 0.5, 0.5));
  EXPECT_DOUBLE_EQ(2*M_LN2, Exponential(0, 1, 0.5));
  EXPECT_DOUBLE_EQ(2*M_LN2, Exponential(1, 1, 0.5));
  EXPECT_DOUBLE_EQ(-2*log(0.75), Exponential(1, 0.5, 0.5));
  EXPECT_DOUBLE_EQ(-2*log(0.875), Exponential(1, 0.25, 0.5));
  EXPECT_DOUBLE_EQ(9.8813129168249309e-324,
                   Exponential(1, 9.8813129168249309e-324, 0.5));
  EXPECT_DOUBLE_EQ(0, Exponential(1, 4.9406564584124654e-324, 0.5));

  // Check a rate above 1, i.e. a scale below 1.
  EXPECT_DOUBLE_EQ(HUGE_VAL, Exponential(0, 4.9406564584124654e-324, 2));
  EXPECT_DOUBLE_EQ(372.2200359606906,
                   Exponential(0, 9.8813129168249309e-324, 2));
  EXPECT_DOUBLE_EQ(1.5*M_LN2, Exponential(0, 0.25, 2));
  EXPECT_DOUBLE_EQ(M_LN2, Exponential(0, 0.5, 2));
  EXPECT_DOUBLE_EQ(0.5*M_LN2, Exponential(0, 1, 2));
  EXPECT_DOUBLE_EQ(0.5*M_LN2, Exponential(1, 1, 2));
  EXPECT_DOUBLE_EQ(-0.5*log(0.75), Exponential(1, 0.5, 2));
  EXPECT_DOUBLE_EQ(-0.5*log(0.875), Exponential(1, 0.25, 2));
  EXPECT_DOUBLE_EQ(9.8813129168249309e-324,
                   Exponential(1, 1.9762625833649862e-323, 2));
  EXPECT_DOUBLE_EQ(9.8813129168249309e-324,
                   Exponential(1, 1.4821969375237396e-323, 2));
  EXPECT_DOUBLE_EQ(0, Exponential(1, 9.8813129168249309e-324, 2));
  EXPECT_DOUBLE_EQ(0, Exponential(1, 4.9406564584124654e-324, 2));
}

TEST_F(BraveRandomDeterministicTest, Geometric) {
  using brave_base::random::deterministic::Geometric;

  // Check a period above 1, i.e. a rate below 1.  Don't bother
  // testing infinities since they can't be converted to integer, and
  // they have negligible^8 probability of turning up anyway.
  EXPECT_EQ(1488ULL, Geometric(0, 9.8813129168249309e-324, 2));
  EXPECT_EQ(4ULL, Geometric(0, 0.25, 2));
  EXPECT_EQ(2ULL, Geometric(0, 0.5, 2));
  EXPECT_EQ(1ULL, Geometric(0, 1, 2));
  EXPECT_EQ(1ULL, Geometric(1, 1, 2));
  EXPECT_EQ(0ULL, Geometric(1, 0.5, 2));
  EXPECT_EQ(0ULL, Geometric(1, 9.8813129168249309e-324, 2));
  EXPECT_EQ(0ULL, Geometric(1, 4.9406564584124654e-324, 2));

  // Check a period below 1, i.e. a rate above 1.  Don't bother
  // testing infinities since they can't be converted to integer, and
  // they have negligible^8 probability of turning up anyway.
  EXPECT_EQ(372ULL, Geometric(0, 9.8813129168249309e-324, 0.5));
  EXPECT_EQ(1ULL, Geometric(0, 0.25, 0.5));
  EXPECT_EQ(0ULL, Geometric(0, 0.5, 0.5));
  EXPECT_EQ(0ULL, Geometric(0, 1, 0.5));
  EXPECT_EQ(0ULL, Geometric(1, 1, 0.5));
  EXPECT_EQ(0ULL, Geometric(1, 0.5, 0.5));
  EXPECT_EQ(0ULL, Geometric(1, 9.8813129168249309e-324, 0.5));
  EXPECT_EQ(0ULL, Geometric(1, 4.9406564584124654e-324, 0.5));
}
