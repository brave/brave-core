/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/ad_block_only_mode_policy_manager.h"

#include <optional>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_origin {

namespace {

class TestAdBlockOnlyModePolicyObserver
    : public AdBlockOnlyModePolicyManager::Observer {
 public:
  ~TestAdBlockOnlyModePolicyObserver() override = default;

  void OnAdBlockOnlyModePoliciesChanged() override {
    ad_block_only_mode_enabled_ =
        AdBlockOnlyModePolicyManager::GetInstance()->IsAdBlockOnlyModeEnabled();
  }

  std::optional<bool> AdblockOnlyModeValueOfLastNotification() const {
    return ad_block_only_mode_enabled_;
  }

 private:
  std::optional<bool> ad_block_only_mode_enabled_;
};

}  // namespace

class AdBlockOnlyModePolicyManagerTest : public testing::Test {
 public:
  AdBlockOnlyModePolicyManagerTest() = default;
  ~AdBlockOnlyModePolicyManagerTest() override = default;

  void SetUp() override {
    local_state_.registry()->RegisterBooleanPref(
        brave_shields::prefs::kAdBlockOnlyModeEnabled, false);
  }

  void TearDown() override { manager()->Shutdown(); }

  AdBlockOnlyModePolicyManager* manager() {
    return AdBlockOnlyModePolicyManager::GetInstance();
  }

  TestingPrefServiceSimple* local_state() { return &local_state_; }

  TestAdBlockOnlyModePolicyObserver* observer() { return &observer_; }

  void EnableFeature() {
    feature_list_.InitAndEnableFeature(
        brave_shields::features::kAdblockOnlyMode);
  }

  void DisableFeature() {
    feature_list_.InitAndDisableFeature(
        brave_shields::features::kAdblockOnlyMode);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
  TestingPrefServiceSimple local_state_;
  TestAdBlockOnlyModePolicyObserver observer_;
};

TEST_F(AdBlockOnlyModePolicyManagerTest,
       IsAdBlockOnlyModeEnabledWhenFeatureDisabled) {
  DisableFeature();
  manager()->Init(local_state());
  local_state()->SetBoolean(brave_shields::prefs::kAdBlockOnlyModeEnabled,
                            true);

  EXPECT_FALSE(manager()->IsAdBlockOnlyModeEnabled());
}

TEST_F(AdBlockOnlyModePolicyManagerTest,
       IsAdBlockOnlyModeEnabledWhenManagerNotInitialized) {
  EnableFeature();
  local_state()->SetBoolean(brave_shields::prefs::kAdBlockOnlyModeEnabled,
                            true);

  EXPECT_FALSE(manager()->IsInitialized());
  EXPECT_FALSE(manager()->IsAdBlockOnlyModeEnabled());
}

TEST_F(AdBlockOnlyModePolicyManagerTest,
       IsAdBlockOnlyModeEnabledWhenPreferenceEnabled) {
  EnableFeature();
  manager()->Init(local_state());
  local_state()->SetBoolean(brave_shields::prefs::kAdBlockOnlyModeEnabled,
                            true);

  EXPECT_TRUE(manager()->IsInitialized());
  EXPECT_TRUE(manager()->IsAdBlockOnlyModeEnabled());
}

TEST_F(AdBlockOnlyModePolicyManagerTest,
       IsAdBlockOnlyModeEnabledWhenPreferenceDisabled) {
  EnableFeature();
  manager()->Init(local_state());
  local_state()->SetBoolean(brave_shields::prefs::kAdBlockOnlyModeEnabled,
                            false);

  EXPECT_TRUE(manager()->IsInitialized());
  EXPECT_FALSE(manager()->IsAdBlockOnlyModeEnabled());
}

TEST_F(AdBlockOnlyModePolicyManagerTest, AddObserverWithFeatureDisabled) {
  DisableFeature();
  manager()->Init(local_state());
  manager()->AddObserver(observer());
  local_state()->SetBoolean(brave_shields::prefs::kAdBlockOnlyModeEnabled,
                            true);

  EXPECT_FALSE(
      observer()->AdblockOnlyModeValueOfLastNotification().has_value());
}

TEST_F(AdBlockOnlyModePolicyManagerTest, AddObserverBeforeInit) {
  EnableFeature();
  manager()->AddObserver(observer());
  EXPECT_FALSE(
      observer()->AdblockOnlyModeValueOfLastNotification().has_value());

  manager()->Init(local_state());
  EXPECT_EQ(observer()->AdblockOnlyModeValueOfLastNotification(), false);

  local_state()->SetBoolean(brave_shields::prefs::kAdBlockOnlyModeEnabled,
                            true);
  EXPECT_EQ(observer()->AdblockOnlyModeValueOfLastNotification(), true);
}

TEST_F(AdBlockOnlyModePolicyManagerTest, AddObserverAfterInit) {
  EnableFeature();
  manager()->Init(local_state());

  manager()->AddObserver(observer());
  EXPECT_EQ(observer()->AdblockOnlyModeValueOfLastNotification(), false);

  local_state()->SetBoolean(brave_shields::prefs::kAdBlockOnlyModeEnabled,
                            true);
  EXPECT_EQ(observer()->AdblockOnlyModeValueOfLastNotification(), true);
}

TEST_F(AdBlockOnlyModePolicyManagerTest,
       AddObserverAfterSetAdBlockOnlyModeEnabled) {
  EnableFeature();
  manager()->Init(local_state());
  local_state()->SetBoolean(brave_shields::prefs::kAdBlockOnlyModeEnabled,
                            true);

  manager()->AddObserver(observer());
  EXPECT_EQ(observer()->AdblockOnlyModeValueOfLastNotification(), true);
}

TEST_F(AdBlockOnlyModePolicyManagerTest, Shutdown) {
  EnableFeature();
  manager()->Init(local_state());
  local_state()->SetBoolean(brave_shields::prefs::kAdBlockOnlyModeEnabled,
                            true);

  manager()->Shutdown();

  EXPECT_FALSE(manager()->IsInitialized());
  EXPECT_FALSE(manager()->IsAdBlockOnlyModeEnabled());
}

}  // namespace brave_origin
