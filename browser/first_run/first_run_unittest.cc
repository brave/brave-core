/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/first_run/first_run.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(FirstRunTest, BasicTest) {
  EXPECT_TRUE(first_run::IsMetricsReportingOptIn());
}
