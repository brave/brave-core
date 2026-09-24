/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/variations/service/buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
typedef testing::Test FieldTrialsTest;

TEST_F(FieldTrialsTest, FieldTrialsTestingEnabled) {
  bool enabled = false;
#if BUILDFLAG(FIELDTRIAL_TESTING_ENABLED)
  enabled = true;
#endif  // BUILDFLAG(FIELDTRIAL_TESTING_ENABLED)
  EXPECT_FALSE(enabled);
}

}  // namespace
