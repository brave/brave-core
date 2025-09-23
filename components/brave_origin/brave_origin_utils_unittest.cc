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
constexpr char kTestGlobalPref[] = "test.global.pref";
constexpr char kTestProfilePref[] = "test.profile.pref";
constexpr char kTestGlobalPolicy[] = "TestGlobalPolicy";
constexpr char kTestProfilePolicy[] = "TestProfilePolicy";
constexpr char kTestGlobalPrefKey[] = "test_global_pref_key";
constexpr char kTestProfilePrefKey[] = "test_profile_pref_key";

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

TEST_F(BraveOriginUtilsTest, GetBraveOriginBrowserPrefKey) {
  BraveOriginPolicyInfo browser_pref_info(
      kTestGlobalPref,    // pref_name
      true,               // default_value
      true,               // user_settable
      kTestGlobalPolicy,  // policy_key
      kTestGlobalPrefKey  // brave_origin_pref_key
  );

  std::string result = GetBraveOriginBrowserPrefKey(browser_pref_info);

  EXPECT_EQ(kTestGlobalPolicy, result);
}

TEST_F(BraveOriginUtilsTest, GetBraveOriginProfilePrefKey) {
  BraveOriginPolicyInfo profile_pref_info(
      kTestProfilePref,    // pref_name
      false,               // default_value
      true,                // user_settable
      kTestProfilePolicy,  // policy_key
      kTestProfilePrefKey  // brave_origin_pref_key
  );

  std::string result =
      GetBraveOriginProfilePrefKey(profile_pref_info, "profile123");

  EXPECT_EQ("profile123.TestProfilePolicy", result);
}

TEST_F(BraveOriginUtilsTest, GetBraveOriginProfilePrefKey_EmptyProfileId) {
  BraveOriginPolicyInfo profile_pref_info(
      kTestProfilePref,    // pref_name
      false,               // default_value
      false,               // user_settable
      kTestProfilePolicy,  // policy_key
      kTestProfilePrefKey  // brave_origin_pref_key
  );

  EXPECT_DEATH_IF_SUPPORTED(GetBraveOriginProfilePrefKey(profile_pref_info, ""),
                            ".*");
}

TEST_F(BraveOriginUtilsTest, GetBraveOriginProfilePrefKey_SpecialCharacters) {
  BraveOriginPolicyInfo profile_pref_info(
      kTestProfilePref,    // pref_name
      true,                // default_value
      true,                // user_settable
      "Test-Policy_Key",   // policy_key with special chars
      kTestProfilePrefKey  // brave_origin_pref_key
  );

  std::string result =
      GetBraveOriginProfilePrefKey(profile_pref_info, "Profile-1_test");

  EXPECT_EQ("Profile-1_test.Test-Policy_Key", result);
}

}  // namespace brave_origin
