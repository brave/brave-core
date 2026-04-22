/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_TIME_PERIOD_STORAGE_TEST_SCOPED_TIMEZONE_FOR_TESTING_H_
#define BRAVE_COMPONENTS_SERP_METRICS_TIME_PERIOD_STORAGE_TEST_SCOPED_TIMEZONE_FOR_TESTING_H_

#include <string>
#include <string_view>

#include "base/test/icu_test_util.h"
#include "base/test/scoped_libc_timezone_override.h"

namespace serp_metrics::test {

// Overrides both the libc and ICU timezones for the duration of a test scope.
// Chromium date operations use both, so setting only one produces inconsistent
// results for callers of `LocalMidnight()` and ICU date formatting functions.

class ScopedTimezoneForTesting final {
 public:
  explicit ScopedTimezoneForTesting(std::string_view timezone_id)
      : timezone_(timezone_id), libc_(timezone_), icu_(timezone_.c_str()) {}

  ScopedTimezoneForTesting(const ScopedTimezoneForTesting&) = delete;
  ScopedTimezoneForTesting& operator=(const ScopedTimezoneForTesting&) = delete;

  ~ScopedTimezoneForTesting() = default;

 private:
  const std::string timezone_;
  const base::test::ScopedLibcTimezoneOverride libc_;
  const base::test::ScopedRestoreDefaultTimezone icu_;
};

}  // namespace serp_metrics::test

#endif  // BRAVE_COMPONENTS_SERP_METRICS_TIME_PERIOD_STORAGE_TEST_SCOPED_TIMEZONE_FOR_TESTING_H_
