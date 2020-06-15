/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p2a/brave_p2a_protocols.h"

#include <math.h>
#include <stdint.h>

#include <vector>

#include "base/rand_util.h"

namespace {
const double kEpsilon = 2.1;
}  // namespace

namespace brave {

DirectEncodingProtocol::DirectEncodingProtocol() = default;

DirectEncodingProtocol::~DirectEncodingProtocol() = default;

uint64_t DirectEncodingProtocol::Perturb(
    uint16_t bucket_count, uint64_t value) {
  uint64_t perturbed_value = value;
  // TODO(Moritz Haller): Ensure no division by zero?
  double probability = exp(kEpsilon) / (exp(kEpsilon) + bucket_count - 1);

  // Return true value with probability
  if (base::RandDouble() < probability) {
      return perturbed_value;
  }

  // Generate set of non-truthful options by removing the true bucket from the
  // candidate vector
  std::vector<uint16_t> buckets;
  for (uint16_t bucket = 0; bucket < bucket_count; bucket++) {
    buckets.push_back(bucket);
  }
  buckets.erase(buckets.begin() + value);

  // Now pick non-truthful bucket at uniform random
  const int rand = base::RandInt(0, bucket_count - 2);
  perturbed_value = buckets.at(rand);

  return perturbed_value;
}

}  // namespace brave
