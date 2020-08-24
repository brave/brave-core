/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../../components/sync/base/model_type_unittest.cc"

namespace syncer {
namespace {

TEST_F(ModelTypeTest, EncryptableUserTypes) {
  EXPECT_TRUE(EncryptableUserTypes().Has(DEVICE_INFO));
}

// This test is supposed to fail when sync types are increased/decreased
TEST_F(ModelTypeTest, ModelTypeCounts) {
  EXPECT_EQ(static_cast<int>(ModelTypeForHistograms::kMaxValue), 49);
}

}  // namespace
}  // namespace syncer
