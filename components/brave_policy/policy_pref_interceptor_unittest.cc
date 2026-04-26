/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_policy/policy_pref_interceptor.h"

#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "components/prefs/pref_value_map.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_policy {

namespace {
constexpr char kUnmanagedPref[] = "some.unmanaged.pref";
}  // namespace

class PolicyPrefInterceptorTest : public ::testing::Test {
 protected:
  PolicyPrefInterceptor interceptor_;
};

TEST_F(PolicyPrefInterceptorTest, CachesInitialValuesAndBlocksChanges) {
  PrefValueMap initial_map;
  initial_map.SetBoolean(brave_rewards::prefs::kDisabledByPolicy, true);
  initial_map.SetBoolean(brave_news::prefs::kBraveNewsDisabledByPolicy, false);
  interceptor_.InterceptPrefValues(&initial_map);

  bool value;
  ASSERT_TRUE(
      initial_map.GetBoolean(brave_rewards::prefs::kDisabledByPolicy, &value));
  EXPECT_TRUE(value);
  ASSERT_TRUE(initial_map.GetBoolean(
      brave_news::prefs::kBraveNewsDisabledByPolicy, &value));
  EXPECT_FALSE(value);

  PrefValueMap updated_map;
  updated_map.SetBoolean(brave_rewards::prefs::kDisabledByPolicy, false);
  updated_map.SetBoolean(brave_news::prefs::kBraveNewsDisabledByPolicy, true);
  interceptor_.InterceptPrefValues(&updated_map);

  ASSERT_TRUE(
      updated_map.GetBoolean(brave_rewards::prefs::kDisabledByPolicy, &value));
  EXPECT_TRUE(value);
  ASSERT_TRUE(updated_map.GetBoolean(
      brave_news::prefs::kBraveNewsDisabledByPolicy, &value));
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
  EXPECT_FALSE(
      initial_map.GetBoolean(brave_rewards::prefs::kDisabledByPolicy, &value));

  PrefValueMap updated_map;
  updated_map.SetBoolean(brave_rewards::prefs::kDisabledByPolicy, true);
  interceptor_.InterceptPrefValues(&updated_map);

  EXPECT_FALSE(
      updated_map.GetBoolean(brave_rewards::prefs::kDisabledByPolicy, &value));
}

}  // namespace brave_policy
