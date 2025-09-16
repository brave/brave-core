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

TEST_F(BraveOriginUtilsTest, GetBraveOriginPrefKey_GlobalScope) {
  BraveOriginPolicyInfo global_pref_info(
      "test.global.pref",               // pref_name
      true,                             // default_value
      BraveOriginPolicyScope::kGlobal,  // scope
      true,                             // user_settable
      "TestGlobalPolicy",               // policy_key
      "test_global_pref_key"            // brave_origin_pref_key
  );

  std::string result = GetBraveOriginPrefKey(global_pref_info, "profile123");

  EXPECT_EQ("TestGlobalPolicy", result);
}

TEST_F(BraveOriginUtilsTest, GetBraveOriginPrefKey_ProfileScope) {
  BraveOriginPolicyInfo profile_pref_info(
      "test.profile.pref",               // pref_name
      false,                             // default_value
      BraveOriginPolicyScope::kProfile,  // scope
      true,                              // user_settable
      "TestProfilePolicy",               // policy_key
      "test_profile_pref_key"            // brave_origin_pref_key
  );

  std::string result = GetBraveOriginPrefKey(profile_pref_info, "profile123");

  EXPECT_EQ("profile123.TestProfilePolicy", result);
}

TEST_F(BraveOriginUtilsTest, GetBraveOriginPrefKey_ProfileScopeEmptyProfileId) {
  BraveOriginPolicyInfo profile_pref_info(
      "test.profile.pref",               // pref_name
      false,                             // default_value
      BraveOriginPolicyScope::kProfile,  // scope
      false,                             // user_settable
      "TestProfilePolicy",               // policy_key
      "test_profile_pref_key"            // brave_origin_pref_key
  );

  EXPECT_DEATH_IF_SUPPORTED(GetBraveOriginPrefKey(profile_pref_info, ""), ".*");
}

TEST_F(BraveOriginUtilsTest,
       GetBraveOriginPrefKey_ProfileScopeSpecialCharacters) {
  BraveOriginPolicyInfo profile_pref_info(
      "test.profile.pref",               // pref_name
      true,                              // default_value
      BraveOriginPolicyScope::kProfile,  // scope
      true,                              // user_settable
      "Test-Policy_Key",                 // policy_key with special chars
      "test_profile_pref_key"            // brave_origin_pref_key
  );

  std::string result =
      GetBraveOriginPrefKey(profile_pref_info, "Profile-1_test");

  EXPECT_EQ("Profile-1_test.Test-Policy_Key", result);
}

}  // namespace brave_origin
