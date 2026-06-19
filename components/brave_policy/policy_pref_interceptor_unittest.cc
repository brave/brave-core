/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_policy/policy_pref_interceptor.h"

#include <string_view>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_policy/features.h"
#include "brave/components/brave_policy/policy_pref_interceptor_list.h"
#include "components/prefs/pref_value_map.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_policy {

namespace {
constexpr std::string_view kManagedPref1 = "test.managed.pref1";
constexpr std::string_view kManagedPref2 = "test.managed.pref2";
constexpr std::string_view kUnmanagedPref = "test.unmanaged.pref";

constexpr std::string_view kTestPrefs[] = {kManagedPref1, kManagedPref2};
}  // namespace

class PolicyPrefInterceptorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    PolicyPrefInterceptorList::GetInstance()->SetPrefs(kTestPrefs);
  }

  void TearDown() override {
    PolicyPrefInterceptorList::GetInstance()->SetPrefs({});
  }

  PolicyPrefInterceptor interceptor_;
};

TEST_F(PolicyPrefInterceptorTest, CachesInitialValuesAndBlocksChanges) {
  PrefValueMap initial_map;
  initial_map.SetBoolean(kManagedPref1, true);
  initial_map.SetBoolean(kManagedPref2, false);
  interceptor_.InterceptPrefValues(&initial_map);

  bool value;
  ASSERT_TRUE(initial_map.GetBoolean(kManagedPref1, &value));
  EXPECT_TRUE(value);
  ASSERT_TRUE(initial_map.GetBoolean(kManagedPref2, &value));
  EXPECT_FALSE(value);

  PrefValueMap updated_map;
  updated_map.SetBoolean(kManagedPref1, false);
  updated_map.SetBoolean(kManagedPref2, true);
  interceptor_.InterceptPrefValues(&updated_map);

  ASSERT_TRUE(updated_map.GetBoolean(kManagedPref1, &value));
  EXPECT_TRUE(value);
  ASSERT_TRUE(updated_map.GetBoolean(kManagedPref2, &value));
  EXPECT_FALSE(value);
}

TEST_F(PolicyPrefInterceptorTest, LeavesUnmanagedPrefsUntouched) {
  PrefValueMap initial_map;
  initial_map.SetBoolean(kUnmanagedPref, true);
  interceptor_.InterceptPrefValues(&initial_map);

  bool value;
  ASSERT_TRUE(initial_map.GetBoolean(kUnmanagedPref, &value));
  EXPECT_TRUE(value);

  PrefValueMap updated_map;
  updated_map.SetBoolean(kUnmanagedPref, false);
  interceptor_.InterceptPrefValues(&updated_map);

  ASSERT_TRUE(updated_map.GetBoolean(kUnmanagedPref, &value));
  EXPECT_FALSE(value);
}

TEST_F(PolicyPrefInterceptorTest, RemovesNewlyAddedPrefs) {
  PrefValueMap initial_map;
  interceptor_.InterceptPrefValues(&initial_map);

  bool value;
  EXPECT_FALSE(initial_map.GetBoolean(kManagedPref1, &value));

  PrefValueMap updated_map;
  updated_map.SetBoolean(kManagedPref1, true);
  interceptor_.InterceptPrefValues(&updated_map);

  EXPECT_FALSE(updated_map.GetBoolean(kManagedPref1, &value));
}

TEST_F(PolicyPrefInterceptorTest, DoesNotInterceptWhenFeatureDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(features::kCacheNonDynamicPolicyPrefs);

  PrefValueMap initial_map;
  initial_map.SetBoolean(kManagedPref1, true);
  interceptor_.InterceptPrefValues(&initial_map);

  // Subsequent call should not restore cached values since feature is disabled.
  PrefValueMap updated_map;
  updated_map.SetBoolean(kManagedPref1, false);
  interceptor_.InterceptPrefValues(&updated_map);

  bool value;
  ASSERT_TRUE(updated_map.GetBoolean(kManagedPref1, &value));
  EXPECT_FALSE(value);
}

}  // namespace brave_policy
