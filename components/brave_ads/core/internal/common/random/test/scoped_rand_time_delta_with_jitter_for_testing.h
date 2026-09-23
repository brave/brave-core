/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RANDOM_TEST_SCOPED_RAND_TIME_DELTA_WITH_JITTER_FOR_TESTING_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RANDOM_TEST_SCOPED_RAND_TIME_DELTA_WITH_JITTER_FOR_TESTING_H_

#include <optional>

#include "base/time/time.h"

namespace brave_ads {

// Sets the value returned by `RandTimeDeltaWithJitter` for testing. Pass
// `std::nullopt` to restore the default behavior.
void SetRandTimeDeltaWithJitterForTesting(std::optional<base::TimeDelta> value);

}  // namespace brave_ads

namespace brave_ads::test {

// Overrides the value returned by `RandTimeDeltaWithJitter` for the duration
// of the test, restoring the original behavior upon destruction.
class ScopedRandTimeDeltaWithJitterForTesting final {
 public:
  explicit ScopedRandTimeDeltaWithJitterForTesting(base::TimeDelta time_delta);

  ScopedRandTimeDeltaWithJitterForTesting(
      const ScopedRandTimeDeltaWithJitterForTesting&) = delete;
  ScopedRandTimeDeltaWithJitterForTesting& operator=(
      const ScopedRandTimeDeltaWithJitterForTesting&) = delete;

  ~ScopedRandTimeDeltaWithJitterForTesting();
};

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RANDOM_TEST_SCOPED_RAND_TIME_DELTA_WITH_JITTER_FOR_TESTING_H_
