/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_utils.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_origin/features.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_origin {

class BraveOriginUtilsTest : public testing::Test {
 public:
  BraveOriginUtilsTest() = default;
  ~BraveOriginUtilsTest() override = default;

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(BraveOriginUtilsTest, IsBraveOriginEnabled_FeatureDisabled) {
  scoped_feature_list_.InitAndDisableFeature(features::kBraveOrigin);

  EXPECT_FALSE(IsBraveOriginEnabled());
}

TEST_F(BraveOriginUtilsTest, IsBraveOriginEnabled_FeatureEnabled) {
  scoped_feature_list_.InitAndEnableFeature(features::kBraveOrigin);

#if DCHECK_IS_ON()  // Debug builds only
  EXPECT_TRUE(IsBraveOriginEnabled());
#else
  EXPECT_FALSE(IsBraveOriginEnabled());  // Always disabled in release builds
#endif
}

}  // namespace brave_origin