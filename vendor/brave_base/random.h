/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BASE_RANDOM_H_
#define BRAVE_BASE_RANDOM_H_

#include <stdint.h>

namespace brave_base {
namespace random {

// WARNING: These routines DO NOT necessarily run in constant time.

// Uniform random 64-bit integer.
uint64_t Uniform64();

// Uniform random real number in [0,1], rounded to double.
//
// Effectively restricted to (0, 1] because probability of returning 0
// is 2^-1075.  Floating-point has high precision near 0, low
// precision near 1; if you are tempted to want [0, 1) instead by
// analogy with integers, fix your downstream code -- e.g.,
// log(Uniform_01()) is safe, but log(base::RandDouble()) is not.
double Uniform_01();

// Exponential distribution.  Supported on positive real numbers, with
// probability density function
//
//      f(k) = e^{-x/rate} / rate.
//
// Return value is in reciprocal units of rate parameter.
//
// Use this to choose a continuous waiting time between events with a
// prescribed average rate of events per unit of time.  For example,
// set a timer for Exponential(1.5) minutes if you want there to be an
// average of 1.5 events per minute.
double Exponential(double rate);

// Geometric distribution.  Supported on _nonnegative_ integers, with
// probability mass function
//
//      P(k) = p*(1 - p)^k,    where    p = 1 - e^{-1/period}.
//
// Return value is in the same units as period parameter.
//
// Note that the parameter is _not_ p = P(0), the probability of a
// success on the first trial; rather it is the average period between
// successes, which is -log(1 - p).  In statistics jargon, this might
// more conventionally be called a scale parameter, but it serves
// functionally as an average period between events in the same units
// as the result.
//
// Use this to choose a discrete number of units of time to wait
// between events with a prescribed average period between events.
// For example, set a timer for Geometric(15*60) seconds if you want
// there to be an average of one event every fifteen minutes.
uint64_t Geometric(double period);

// Internal namespace for testing.

namespace deterministic {

// Deterministic transforms.  These map uniform (or geometric)
// distribution on cartesian products of Z/(2^64)Z and [0, 1] into
// various other distributions.  These facilitate deterministic
// automatic testing of the numerical analysis, but are probably not
// what you're looking for if you just need to roll a die.

double Uniform_01(uint64_t e, uint64_t u);
double StdExponential(uint64_t s, double p0);
double Exponential(uint64_t s, double p0, double rate);
uint64_t Geometric(uint64_t s, double p0, double period);

}  // namespace deterministic

}  // namespace random
}  // namespace brave_base

#endif  // BRAVE_BASE_RANDOM_H_
