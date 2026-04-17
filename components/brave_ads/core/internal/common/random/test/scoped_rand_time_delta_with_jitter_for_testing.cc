/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/random/test/scoped_rand_time_delta_with_jitter_for_testing.h"

namespace brave_ads::test {

ScopedRandTimeDeltaWithJitterForTesting::
    ScopedRandTimeDeltaWithJitterForTesting(base::TimeDelta time_delta) {
  SetRandTimeDeltaWithJitterForTesting(time_delta);
}

ScopedRandTimeDeltaWithJitterForTesting::
    ~ScopedRandTimeDeltaWithJitterForTesting() {
  SetRandTimeDeltaWithJitterForTesting(std::nullopt);
}

}  // namespace brave_ads::test
