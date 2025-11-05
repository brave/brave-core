/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_utils.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_origin/brave_origin_policy_info.h"
#include "brave/components/brave_origin/features.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_origin {

// Test constants
constexpr char kTestGlobalPolicy[] = "TestGlobalPolicy";
constexpr char kTestProfilePolicy[] = "TestProfilePolicy";

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

  EXPECT_TRUE(IsBraveOriginEnabled());
}

TEST_F(BraveOriginUtilsTest, GetBraveOriginPrefKey_BrowserPolicy) {
  std::string result = GetBraveOriginPrefKey(kTestGlobalPolicy, std::nullopt);

  EXPECT_EQ(kTestGlobalPolicy, result);
}

TEST_F(BraveOriginUtilsTest, GetBraveOriginPrefKey_ProfilePolicy) {
  std::string result = GetBraveOriginPrefKey(kTestProfilePolicy, "profile123");

  EXPECT_EQ("profile123.TestProfilePolicy", result);
}

TEST_F(BraveOriginUtilsTest, GetBraveOriginPrefKey_SpecialCharacters) {
  std::string result =
      GetBraveOriginPrefKey("Test-Policy_Key", "Profile-1_test");

  EXPECT_EQ("Profile-1_test.Test-Policy_Key", result);
}

}  // namespace brave_origin
