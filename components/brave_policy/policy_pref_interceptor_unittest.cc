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
  interceptor_.InterceptPrefValues(&initial_map,
                                   /*policies_initialized=*/false);

  bool value;
  ASSERT_TRUE(initial_map.GetBoolean(kManagedPref1, &value));
  EXPECT_TRUE(value);
  ASSERT_TRUE(initial_map.GetBoolean(kManagedPref2, &value));
  EXPECT_FALSE(value);

  PrefValueMap updated_map;
  updated_map.SetBoolean(kManagedPref1, false);
  updated_map.SetBoolean(kManagedPref2, true);
  interceptor_.InterceptPrefValues(&updated_map,
                                   /*policies_initialized=*/true);

  ASSERT_TRUE(updated_map.GetBoolean(kManagedPref1, &value));
  EXPECT_TRUE(value);
  ASSERT_TRUE(updated_map.GetBoolean(kManagedPref2, &value));
  EXPECT_FALSE(value);
}

TEST_F(PolicyPrefInterceptorTest, LeavesUnmanagedPrefsUntouched) {
  PrefValueMap initial_map;
  initial_map.SetBoolean(kUnmanagedPref, true);
  interceptor_.InterceptPrefValues(&initial_map,
                                   /*policies_initialized=*/false);

  bool value;
  ASSERT_TRUE(initial_map.GetBoolean(kUnmanagedPref, &value));
  EXPECT_TRUE(value);

  PrefValueMap updated_map;
  updated_map.SetBoolean(kUnmanagedPref, false);
  interceptor_.InterceptPrefValues(&updated_map,
                                   /*policies_initialized=*/true);

  ASSERT_TRUE(updated_map.GetBoolean(kUnmanagedPref, &value));
  EXPECT_FALSE(value);
}

TEST_F(PolicyPrefInterceptorTest, RemovesNewlyAddedPrefs) {
  PrefValueMap initial_map;
  interceptor_.InterceptPrefValues(&initial_map,
                                   /*policies_initialized=*/false);

  bool value;
  EXPECT_FALSE(initial_map.GetBoolean(kManagedPref1, &value));

  PrefValueMap updated_map;
  updated_map.SetBoolean(kManagedPref1, true);
  interceptor_.InterceptPrefValues(&updated_map,
                                   /*policies_initialized=*/true);

  EXPECT_FALSE(updated_map.GetBoolean(kManagedPref1, &value));
}

TEST_F(PolicyPrefInterceptorTest, DoesNotModifyMapBeforeInitialization) {
  // While policies are loading, the map is never modified even if values
  // differ from what is cached.
  PrefValueMap loading_map;
  loading_map.SetBoolean(kManagedPref1, true);
  interceptor_.InterceptPrefValues(&loading_map,
                                   /*policies_initialized=*/false);

  bool value;
  ASSERT_TRUE(loading_map.GetBoolean(kManagedPref1, &value));
  EXPECT_TRUE(value);

  PrefValueMap loading_map2;
  loading_map2.SetBoolean(kManagedPref1, false);
  interceptor_.InterceptPrefValues(&loading_map2,
                                   /*policies_initialized=*/false);

  ASSERT_TRUE(loading_map2.GetBoolean(kManagedPref1, &value));
  EXPECT_FALSE(value);
}

TEST_F(PolicyPrefInterceptorTest, EmptySnapshotIsNotPersisted) {
  // An empty refresh before initialization (e.g. the platform provider has not
  // yet delivered policy values) must not be frozen as the snapshot.
  PrefValueMap empty_map;
  interceptor_.InterceptPrefValues(&empty_map,
                                   /*policies_initialized=*/false);

  // The real values arrive in a later pre-init refresh and update the cache.
  PrefValueMap loaded_map;
  loaded_map.SetBoolean(kManagedPref1, true);
  loaded_map.SetBoolean(kManagedPref2, false);
  interceptor_.InterceptPrefValues(&loaded_map,
                                   /*policies_initialized=*/false);

  // After initialization, the cached real values are enforced and preserved
  // against later changes, rather than the prefs being stripped.
  PrefValueMap updated_map;
  updated_map.SetBoolean(kManagedPref1, false);
  updated_map.SetBoolean(kManagedPref2, true);
  interceptor_.InterceptPrefValues(&updated_map,
                                   /*policies_initialized=*/true);

  bool value;
  ASSERT_TRUE(updated_map.GetBoolean(kManagedPref1, &value));
  EXPECT_TRUE(value);
  ASSERT_TRUE(updated_map.GetBoolean(kManagedPref2, &value));
  EXPECT_FALSE(value);
}

TEST_F(PolicyPrefInterceptorTest, EvictsPrefsRemovedWhileLoading) {
  // A pref cached early during loading should be evicted if it disappears from
  // a later pre-init refresh, so it isn't enforced after initialization.
  PrefValueMap loading_map;
  loading_map.SetBoolean(kManagedPref1, true);
  interceptor_.InterceptPrefValues(&loading_map,
                                   /*policies_initialized=*/false);

  PrefValueMap loading_map2;
  interceptor_.InterceptPrefValues(&loading_map2,
                                   /*policies_initialized=*/false);

  PrefValueMap updated_map;
  updated_map.SetBoolean(kManagedPref1, true);
  interceptor_.InterceptPrefValues(&updated_map,
                                   /*policies_initialized=*/true);

  bool value;
  EXPECT_FALSE(updated_map.GetBoolean(kManagedPref1, &value));
}

TEST_F(PolicyPrefInterceptorTest, DoesNotOverridePrefsWhenFeatureDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(features::kCacheNonDynamicPolicyPrefs);

  PrefValueMap initial_map;
  initial_map.SetBoolean(kManagedPref1, true);
  interceptor_.InterceptPrefValues(&initial_map,
                                   /*policies_initialized=*/false);

  // Post-init: the map has a changed value; with the feature disabled it should
  // not be overridden.
  PrefValueMap updated_map;
  updated_map.SetBoolean(kManagedPref1, false);
  interceptor_.InterceptPrefValues(&updated_map,
                                   /*policies_initialized=*/true);

  bool value;
  ASSERT_TRUE(updated_map.GetBoolean(kManagedPref1, &value));
  EXPECT_FALSE(value);
}

}  // namespace brave_policy
