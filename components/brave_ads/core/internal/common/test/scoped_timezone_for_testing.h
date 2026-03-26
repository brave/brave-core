/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_SCOPED_TIMEZONE_FOR_TESTING_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_SCOPED_TIMEZONE_FOR_TESTING_H_

#include <string>
#include <string_view>

#include "base/test/icu_test_util.h"
#include "base/test/scoped_libc_timezone_override.h"

namespace brave_ads::test {

// Scoped wrapper that sets both ICU and libc timezones for cross-platform use.
//
// Not thread safe.

class ScopedTimezoneForTesting {
 public:
  explicit ScopedTimezoneForTesting(std::string_view timezone)
      : timezone_(timezone), libc_(timezone_), icu_(timezone_.c_str()) {}

  ScopedTimezoneForTesting(const ScopedTimezoneForTesting&) = delete;
  ScopedTimezoneForTesting& operator=(const ScopedTimezoneForTesting&) = delete;

  ~ScopedTimezoneForTesting() = default;

 private:
  const std::string timezone_;
  const base::test::ScopedLibcTimezoneOverride libc_;
  const base::test::ScopedRestoreDefaultTimezone icu_;
};

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_SCOPED_TIMEZONE_FOR_TESTING_H_
