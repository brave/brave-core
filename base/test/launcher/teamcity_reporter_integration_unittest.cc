/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/check.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

TEST(DISABLED_TeamcityReporterIntegrationTest, Success) {
  EXPECT_TRUE(true);
}

TEST(DISABLED_TeamcityReporterIntegrationTest, Failure) {
  EXPECT_TRUE(false);
}

TEST(DISABLED_TeamcityReporterIntegrationTest, CheckFailure) {
  CHECK(false);
}

TEST(DISABLED_TeamcityReporterIntegrationTest, Skipped) {
  EXPECT_TRUE(true);
}

}  // namespace base
