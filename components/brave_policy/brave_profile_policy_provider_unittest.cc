/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_policy/brave_profile_policy_provider.h"

#include <memory>

#include "base/test/task_environment.h"
#include "components/policy/core/common/policy_bundle.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/core/common/schema_registry.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_policy {

class BraveProfilePolicyProviderTest : public ::testing::Test {
 public:
  BraveProfilePolicyProviderTest() = default;
  ~BraveProfilePolicyProviderTest() override = default;

  void TearDown() override {
    if (provider_.IsInitializationComplete(policy::POLICY_DOMAIN_CHROME)) {
      provider_.Shutdown();
    }
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  policy::SchemaRegistry schema_registry_;
  BraveProfilePolicyProvider provider_;
};

TEST_F(BraveProfilePolicyProviderTest, InitAndPolicyLoadComplete) {
  // Initially, policies should not be loaded
  EXPECT_FALSE(
      provider_.IsFirstPolicyLoadComplete(policy::POLICY_DOMAIN_CHROME));

  // Initialize the provider
  provider_.Init(&schema_registry_);

  // Policies should still not be loaded until observer event fires
  EXPECT_FALSE(
      provider_.IsFirstPolicyLoadComplete(policy::POLICY_DOMAIN_CHROME));

  // Fire the observer event and set profile ID to trigger policy loading
  provider_.OnBraveOriginPoliciesReady();
  provider_.SetProfileID("test-profile-id");

  // Now policies should be loaded
  EXPECT_TRUE(
      provider_.IsFirstPolicyLoadComplete(policy::POLICY_DOMAIN_CHROME));
}

TEST_F(BraveProfilePolicyProviderTest, EmptyPolicyBundle) {
  // Initialize the provider
  provider_.Init(&schema_registry_);

  // Fire the observer event and set profile ID to trigger policy loading
  provider_.OnBraveOriginPoliciesReady();
  provider_.SetProfileID("test-profile-id");

  // Get the policy bundle
  const policy::PolicyBundle& bundle = provider_.policies();

  // Currently the provider returns an empty bundle since no policies are
  // implemented yet
  const policy::PolicyMap& chrome_policies = bundle.Get(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));

  EXPECT_TRUE(chrome_policies.empty())
      << "Policy bundle should be empty as no policies are implemented yet";
}

TEST_F(BraveProfilePolicyProviderTest, RefreshPolicies) {
  // Initialize the provider
  provider_.Init(&schema_registry_);

  // Fire the observer event and set profile ID to trigger policy loading
  provider_.OnBraveOriginPoliciesReady();
  provider_.SetProfileID("test-profile-id");

  // Policies should be loaded after observer event
  EXPECT_TRUE(
      provider_.IsFirstPolicyLoadComplete(policy::POLICY_DOMAIN_CHROME));

  // Refresh policies with different reasons
  provider_.RefreshPolicies(policy::PolicyFetchReason::kUnspecified);
  EXPECT_TRUE(
      provider_.IsFirstPolicyLoadComplete(policy::POLICY_DOMAIN_CHROME));

  provider_.RefreshPolicies(policy::PolicyFetchReason::kTest);
  EXPECT_TRUE(
      provider_.IsFirstPolicyLoadComplete(policy::POLICY_DOMAIN_CHROME));
}

TEST_F(BraveProfilePolicyProviderTest, ShutdownHandling) {
  // Initialize the provider
  provider_.Init(&schema_registry_);

  // Fire the observer event and set profile ID to trigger policy loading
  provider_.OnBraveOriginPoliciesReady();
  provider_.SetProfileID("test-profile-id");

  // Verify initialized state
  EXPECT_TRUE(
      provider_.IsFirstPolicyLoadComplete(policy::POLICY_DOMAIN_CHROME));

  // Shutdown should complete without errors
  provider_.Shutdown();

  // Provider should still report policies as loaded even after shutdown
  EXPECT_TRUE(
      provider_.IsFirstPolicyLoadComplete(policy::POLICY_DOMAIN_CHROME));
}

TEST_F(BraveProfilePolicyProviderTest, PolicyDomainHandling) {
  // Initialize the provider
  provider_.Init(&schema_registry_);

  // Fire the observer event and set profile ID to trigger policy loading
  provider_.OnBraveOriginPoliciesReady();
  provider_.SetProfileID("test-profile-id");

  // Test policy load complete for different domains
  EXPECT_TRUE(
      provider_.IsFirstPolicyLoadComplete(policy::POLICY_DOMAIN_CHROME));
  EXPECT_TRUE(
      provider_.IsFirstPolicyLoadComplete(policy::POLICY_DOMAIN_EXTENSIONS));
  EXPECT_TRUE(provider_.IsFirstPolicyLoadComplete(
      policy::POLICY_DOMAIN_SIGNIN_EXTENSIONS));

  // The provider should report policies as loaded for all domains since it uses
  // first_policies_loaded_ flag uniformly
}

TEST_F(BraveProfilePolicyProviderTest, OnProfilePolicyChanged_MatchingProfile) {
  // Initialize the provider
  provider_.Init(&schema_registry_);

  // Fire the observer event and set profile ID to trigger policy loading
  provider_.OnBraveOriginPoliciesReady();
  provider_.SetProfileID("test-profile-id");

  // Call OnProfilePolicyChanged with matching profile ID
  provider_.OnProfilePolicyChanged("test.pref.policy", "test-profile-id");

  // Provider should still report policies as loaded
  EXPECT_TRUE(
      provider_.IsFirstPolicyLoadComplete(policy::POLICY_DOMAIN_CHROME));
}

TEST_F(BraveProfilePolicyProviderTest,
       OnProfilePolicyChanged_DifferentProfile) {
  // Initialize the provider
  provider_.Init(&schema_registry_);

  // Fire the observer event and set profile ID to trigger policy loading
  provider_.OnBraveOriginPoliciesReady();
  provider_.SetProfileID("test-profile-id");

  // Call OnProfilePolicyChanged with different profile ID
  provider_.OnProfilePolicyChanged("test.pref.policy", "different-profile-id");

  // Provider should still report policies as loaded (no action taken)
  EXPECT_TRUE(
      provider_.IsFirstPolicyLoadComplete(policy::POLICY_DOMAIN_CHROME));
}

TEST_F(BraveProfilePolicyProviderTest, OnProfilePolicyChanged_EmptyProfileId) {
  // Initialize the provider without setting profile ID
  provider_.Init(&schema_registry_);
  provider_.OnBraveOriginPoliciesReady();

  // Call OnProfilePolicyChanged (should not crash or affect anything)
  provider_.OnProfilePolicyChanged("test.pref.policy", "some-profile-id");

  // Provider should still report no policies loaded since no profile ID set
  EXPECT_FALSE(
      provider_.IsFirstPolicyLoadComplete(policy::POLICY_DOMAIN_CHROME));
}

TEST_F(BraveProfilePolicyProviderTest,
       OnAdBlockOnlyModePoliciesChanged_EmptyProfileId) {
  // Initialize the provider
  provider_.Init(&schema_registry_);

  provider_.OnAdBlockOnlyModePoliciesChanged();

  // Provider should report no policies loaded since no profile ID set
  EXPECT_FALSE(
      provider_.IsFirstPolicyLoadComplete(policy::POLICY_DOMAIN_CHROME));
}

TEST_F(BraveProfilePolicyProviderTest,
       OnAdBlockOnlyModePoliciesChanged_NotEmptyProfileId) {
  // Initialize the provider
  provider_.Init(&schema_registry_);
  provider_.SetProfileID("test-profile-id");

  provider_.OnAdBlockOnlyModePoliciesChanged();

  // Provider should still report policies as loaded
  EXPECT_TRUE(
      provider_.IsFirstPolicyLoadComplete(policy::POLICY_DOMAIN_CHROME));
}

}  // namespace brave_policy
