/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/random/random_util.h"

#include <optional>

#include "base/check_is_test.h"
#include "base/rand_util.h"
#include "base/time/time.h"

namespace brave_ads {

namespace {

constexpr double kMinRandomFactor = 0.5;

std::optional<base::TimeDelta> g_rand_time_delta_with_jitter_for_testing;

}  // namespace

base::TimeDelta RandTimeDeltaWithJitter(base::TimeDelta time_delta) {
  if (g_rand_time_delta_with_jitter_for_testing) {
    CHECK_IS_TEST();

    return *g_rand_time_delta_with_jitter_for_testing;
  }

  const double random_factor = kMinRandomFactor + base::RandDouble();

  return base::Seconds(time_delta.InSecondsF() * random_factor);
}

void SetRandTimeDeltaWithJitterForTesting(  // IN-TEST
    std::optional<base::TimeDelta> value) {
  CHECK_IS_TEST();

  g_rand_time_delta_with_jitter_for_testing = value;
}

}  // namespace brave_ads
