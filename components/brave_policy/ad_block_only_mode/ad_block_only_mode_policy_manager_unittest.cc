/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_policy/ad_block_only_mode/ad_block_only_mode_policy_manager.h"

#include <optional>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_policy/brave_policy_observer.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_policy {

namespace {

class TestAdBlockOnlyModePolicyObserver : public BravePolicyObserver {
 public:
  ~TestAdBlockOnlyModePolicyObserver() override = default;

  void OnBravePoliciesReady() override {
    last_ad_block_only_mode_policies_ =
        AdBlockOnlyModePolicyManager::GetInstance()->GetPolicies();
  }

  const std::optional<AdBlockOnlyModePolicies>& LastAdBlockOnlyModePolicies()
      const {
    return last_ad_block_only_mode_policies_;
  }

 private:
  std::optional<AdBlockOnlyModePolicies> last_ad_block_only_mode_policies_;
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

TEST_F(AdBlockOnlyModePolicyManagerTest, PoliciesEmptyWhenFeatureDisabled) {
  DisableFeature();
  manager()->Init(local_state());
  local_state()->SetBoolean(brave_shields::prefs::kAdBlockOnlyModeEnabled,
                            true);

  EXPECT_THAT(manager()->GetPolicies(), testing::IsEmpty());
}

TEST_F(AdBlockOnlyModePolicyManagerTest, PoliciesEmptyBeforeInitCalled) {
  EnableFeature();
  EXPECT_THAT(manager()->GetPolicies(), testing::IsEmpty());
}

TEST_F(AdBlockOnlyModePolicyManagerTest, PoliciesEmptyWhenPreferenceDisabled) {
  EnableFeature();
  manager()->Init(local_state());
  local_state()->SetBoolean(brave_shields::prefs::kAdBlockOnlyModeEnabled,
                            false);

  EXPECT_THAT(manager()->GetPolicies(), testing::IsEmpty());
}

TEST_F(AdBlockOnlyModePolicyManagerTest,
       PoliciesNotEmptyWhenPreferenceEnabled) {
  EnableFeature();
  manager()->Init(local_state());
  local_state()->SetBoolean(brave_shields::prefs::kAdBlockOnlyModeEnabled,
                            true);

  EXPECT_THAT(manager()->GetPolicies(), testing::Not(testing::IsEmpty()));
}

TEST_F(AdBlockOnlyModePolicyManagerTest,
       PoliciesEmptyAfterObserverAddedWhenFeatureDisabled) {
  DisableFeature();
  manager()->Init(local_state());
  manager()->AddObserver(observer());
  local_state()->SetBoolean(brave_shields::prefs::kAdBlockOnlyModeEnabled,
                            true);

  ASSERT_TRUE(observer()->LastAdBlockOnlyModePolicies().has_value());
  EXPECT_THAT(*observer()->LastAdBlockOnlyModePolicies(), testing::IsEmpty());
}

TEST_F(AdBlockOnlyModePolicyManagerTest,
       PoliciesEmptyWhenPreferenceDisabledBeforeObserverAdded) {
  EnableFeature();
  manager()->Init(local_state());
  local_state()->SetBoolean(brave_shields::prefs::kAdBlockOnlyModeEnabled,
                            false);
  manager()->AddObserver(observer());

  ASSERT_TRUE(observer()->LastAdBlockOnlyModePolicies().has_value());
  EXPECT_THAT(*observer()->LastAdBlockOnlyModePolicies(), testing::IsEmpty());
}

TEST_F(AdBlockOnlyModePolicyManagerTest,
       PoliciesEmptyWhenPreferenceDisabledAfterObserverAdded) {
  EnableFeature();
  manager()->Init(local_state());
  manager()->AddObserver(observer());
  local_state()->SetBoolean(brave_shields::prefs::kAdBlockOnlyModeEnabled,
                            false);

  ASSERT_TRUE(observer()->LastAdBlockOnlyModePolicies().has_value());
  EXPECT_THAT(*observer()->LastAdBlockOnlyModePolicies(), testing::IsEmpty());
}

TEST_F(AdBlockOnlyModePolicyManagerTest,
       PoliciesNotEmptyWhenPreferenceEnabledAfterObserverAdded) {
  EnableFeature();
  manager()->Init(local_state());

  manager()->AddObserver(observer());
  ASSERT_TRUE(observer()->LastAdBlockOnlyModePolicies().has_value());
  EXPECT_THAT(*observer()->LastAdBlockOnlyModePolicies(), testing::IsEmpty());

  local_state()->SetBoolean(brave_shields::prefs::kAdBlockOnlyModeEnabled,
                            true);
  EXPECT_THAT(*observer()->LastAdBlockOnlyModePolicies(),
              testing::Not(testing::IsEmpty()));
}

TEST_F(AdBlockOnlyModePolicyManagerTest,
       PoliciesNotEmptyWhenPreferenceEnabledBeforeObserverAdded) {
  EnableFeature();
  manager()->Init(local_state());
  local_state()->SetBoolean(brave_shields::prefs::kAdBlockOnlyModeEnabled,
                            true);

  manager()->AddObserver(observer());

  ASSERT_TRUE(observer()->LastAdBlockOnlyModePolicies().has_value());
  EXPECT_THAT(*observer()->LastAdBlockOnlyModePolicies(),
              testing::Not(testing::IsEmpty()));
}

TEST_F(AdBlockOnlyModePolicyManagerTest, PoliciesEmptyAfterShutdown) {
  EnableFeature();
  manager()->Init(local_state());
  local_state()->SetBoolean(brave_shields::prefs::kAdBlockOnlyModeEnabled,
                            true);
  manager()->Shutdown();

  EXPECT_THAT(manager()->GetPolicies(), testing::IsEmpty());
}

}  // namespace brave_policy
