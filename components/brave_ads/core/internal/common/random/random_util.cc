/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/random/random_util.h"

#include <optional>

#include "base/time/time.h"
#include "brave_base/random.h"

namespace brave_ads {

namespace {
std::optional<base::TimeDelta> g_rand_time_delta_for_testing;
}  // namespace

base::TimeDelta RandTimeDelta(const base::TimeDelta time_delta) {
  if (g_rand_time_delta_for_testing) {
    return *g_rand_time_delta_for_testing;
  }

  return base::Seconds(brave_base::random::Geometric(time_delta.InSecondsF()));
}

ScopedRandTimeDeltaSetterForTesting::ScopedRandTimeDeltaSetterForTesting(
    const base::TimeDelta time_delta) {
  g_rand_time_delta_for_testing = time_delta;
}

ScopedRandTimeDeltaSetterForTesting::~ScopedRandTimeDeltaSetterForTesting() {
  g_rand_time_delta_for_testing = std::nullopt;
}

}  // namespace brave_ads
