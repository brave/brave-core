/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/core/browser/ad_block_only_mode/ad_block_only_mode_policies_loader.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/core/common/policy_bundle.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_shields {

namespace {

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kAdBlockOnlyModeEnabled, false);
}

}  // namespace

class AdBlockOnlyModePoliciesLoaderTest : public ::testing::Test {
 public:
  AdBlockOnlyModePoliciesLoaderTest()
      : configuration_policy_provider_(
            std::make_unique<policy::MockConfigurationPolicyProvider>()) {
    configuration_policy_provider_->Init();

    RegisterLocalStatePrefs(local_state_.registry());
  }

  ~AdBlockOnlyModePoliciesLoaderTest() override {
    configuration_policy_provider_->Shutdown();
  }

 protected:
  std::unique_ptr<policy::MockConfigurationPolicyProvider>
      configuration_policy_provider_;
  TestingPrefServiceSimple local_state_;
};

TEST_F(AdBlockOnlyModePoliciesLoaderTest, NoPoliciesWhenFeatureDisabled) {
  base::test::ScopedFeatureList features;
  features.InitAndDisableFeature(features::kAdblockOnlyMode);
  local_state_.SetBoolean(prefs::kAdBlockOnlyModeEnabled,
                          /*adblock_only_mode_enabled=*/false);

  AdBlockOnlyModePoliciesLoader provider(&local_state_,
                                         *configuration_policy_provider_);
  provider.Init();

  policy::PolicyBundle bundle;
  provider.MaybeLoadPolicies(bundle);

  const policy::PolicyMap& chrome_policies = bundle.Get(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));
  EXPECT_TRUE(chrome_policies.empty());
}

TEST_F(AdBlockOnlyModePoliciesLoaderTest, NoPoliciesWhenPrefDisabled) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeature(features::kAdblockOnlyMode);
  local_state_.SetBoolean(prefs::kAdBlockOnlyModeEnabled, false);

  AdBlockOnlyModePoliciesLoader provider(&local_state_,
                                         *configuration_policy_provider_);
  provider.Init();

  policy::PolicyBundle bundle;
  provider.MaybeLoadPolicies(bundle);
  const policy::PolicyMap& chrome_policies = bundle.Get(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));
  EXPECT_TRUE(chrome_policies.empty());
}

TEST_F(AdBlockOnlyModePoliciesLoaderTest, PoliciesWhenPrefEnabled) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeature(features::kAdblockOnlyMode);
  local_state_.SetBoolean(prefs::kAdBlockOnlyModeEnabled, true);

  AdBlockOnlyModePoliciesLoader provider(&local_state_,
                                         *configuration_policy_provider_);
  provider.Init();

  policy::PolicyBundle bundle;
  provider.MaybeLoadPolicies(bundle);
  const policy::PolicyMap& chrome_policies = bundle.Get(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));
  // Ad Block Only mode policies are currently not supported on iOS.
#if !BUILDFLAG(IS_IOS)
  EXPECT_FALSE(chrome_policies.empty());
#else
  EXPECT_TRUE(chrome_policies.empty());
#endif
}

TEST_F(AdBlockOnlyModePoliciesLoaderTest, PoliciesOnPrefChange) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeature(features::kAdblockOnlyMode);
  local_state_.SetBoolean(prefs::kAdBlockOnlyModeEnabled,
                          /*adblock_only_mode_enabled=*/false);

  AdBlockOnlyModePoliciesLoader provider(&local_state_,
                                         *configuration_policy_provider_);
  provider.Init();

  {
    EXPECT_CALL(*configuration_policy_provider_,
                RefreshPolicies(policy::PolicyFetchReason::kUserRequest))
        .Times(1);
    local_state_.SetBoolean(prefs::kAdBlockOnlyModeEnabled,
                            /*adblock_only_mode_enabled=*/true);
    policy::PolicyBundle bundle;
    provider.MaybeLoadPolicies(bundle);
    const policy::PolicyMap& chrome_policies = bundle.Get(
        policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));
// Ad Block Only mode policies are currently not supported on iOS.
#if !BUILDFLAG(IS_IOS)
    EXPECT_FALSE(chrome_policies.empty());
#else
    EXPECT_TRUE(chrome_policies.empty());
#endif
    testing::Mock::VerifyAndClearExpectations(
        configuration_policy_provider_.get());
  }

  {
    EXPECT_CALL(*configuration_policy_provider_,
                RefreshPolicies(policy::PolicyFetchReason::kUserRequest))
        .Times(1);
    local_state_.SetBoolean(prefs::kAdBlockOnlyModeEnabled,
                            /*adblock_only_mode_enabled=*/false);
    policy::PolicyBundle bundle;
    provider.MaybeLoadPolicies(bundle);
    const policy::PolicyMap& chrome_policies = bundle.Get(
        policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));
    EXPECT_TRUE(chrome_policies.empty());
    testing::Mock::VerifyAndClearExpectations(
        configuration_policy_provider_.get());
  }
}

}  // namespace brave_shields
