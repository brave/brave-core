/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/sync/base/data_type_unittest.cc"

namespace syncer {
namespace {

TEST(DataTypeTest, EncryptableUserTypes) {
  EXPECT_TRUE(EncryptableUserTypes().Has(DEVICE_INFO));
  EXPECT_TRUE(EncryptableUserTypes().Has(HISTORY));
}

TEST(DataTypeTest, LowPriorityUserTypes) {
  EXPECT_TRUE(LowPriorityUserTypes().Has(HISTORY_DELETE_DIRECTIVES));
  EXPECT_FALSE(LowPriorityUserTypes().Has(HISTORY));
  EXPECT_TRUE(LowPriorityUserTypes().Has(USER_EVENTS));
}

// This test is supposed to fail when sync types are increased/decreased
TEST(DataTypeTest, DataTypeCounts) {
  EXPECT_EQ(static_cast<int>(DataTypeForHistograms::kMaxValue), 68);
}

}  // namespace
}  // namespace syncer
