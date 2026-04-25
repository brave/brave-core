/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_policy/brave_browser_policy_provider.h"

#include <memory>

#include "base/test/task_environment.h"
#include "components/policy/core/common/policy_bundle.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/core/common/schema_registry.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_policy {

class BraveBrowserPolicyProviderTest : public ::testing::Test {
 public:
  BraveBrowserPolicyProviderTest() = default;
  ~BraveBrowserPolicyProviderTest() override = default;

  void TearDown() override { provider_.Shutdown(); }

 protected:
  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
  policy::SchemaRegistry schema_registry_;
  BraveBrowserPolicyProvider provider_;
};

TEST_F(BraveBrowserPolicyProviderTest, InitAndPolicyLoadComplete) {
  // Initially, policies should not be loaded
  EXPECT_FALSE(
      provider_.IsFirstPolicyLoadComplete(policy::POLICY_DOMAIN_CHROME));

  // Initialize the provider
  provider_.Init(&schema_registry_);

  // Policies should still not be loaded until observer event fires
  EXPECT_FALSE(
      provider_.IsFirstPolicyLoadComplete(policy::POLICY_DOMAIN_CHROME));

  // Fire the observer event to trigger policy loading
  provider_.OnBravePoliciesReady();

  // Now policies should be loaded
  EXPECT_TRUE(
      provider_.IsFirstPolicyLoadComplete(policy::POLICY_DOMAIN_CHROME));
}

TEST_F(BraveBrowserPolicyProviderTest, EmptyPolicyBundle) {
  // Initialize the provider
  provider_.Init(&schema_registry_);

  // Fire the observer event to trigger policy loading
  provider_.OnBravePoliciesReady();

  // Get the policy bundle
  const policy::PolicyBundle& bundle = provider_.policies();

  // Currently the provider returns an empty bundle since no policies are
  // implemented yet
  const policy::PolicyMap& chrome_policies = bundle.Get(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));

  EXPECT_TRUE(chrome_policies.empty())
      << "Policy bundle should be empty as no policies are implemented yet";
}

TEST_F(BraveBrowserPolicyProviderTest, RefreshPolicies) {
  // Initialize the provider
  provider_.Init(&schema_registry_);

  // Fire the observer event to trigger policy loading
  provider_.OnBravePoliciesReady();

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

TEST_F(BraveBrowserPolicyProviderTest, ShutdownHandling) {
  // Initialize the provider
  provider_.Init(&schema_registry_);

  // Fire the observer event to trigger policy loading
  provider_.OnBravePoliciesReady();

  // Verify initialized state
  EXPECT_TRUE(
      provider_.IsFirstPolicyLoadComplete(policy::POLICY_DOMAIN_CHROME));

  // Shutdown should complete without errors
  provider_.Shutdown();

  // Provider should still report policies as loaded even after shutdown
  EXPECT_TRUE(
      provider_.IsFirstPolicyLoadComplete(policy::POLICY_DOMAIN_CHROME));
}

TEST_F(BraveBrowserPolicyProviderTest, BasicInitialization) {
  // Basic initialization should succeed
  provider_.Init(&schema_registry_);

  // Fire the observer event to trigger policy loading
  provider_.OnBravePoliciesReady();

  EXPECT_TRUE(
      provider_.IsFirstPolicyLoadComplete(policy::POLICY_DOMAIN_CHROME));
}

TEST_F(BraveBrowserPolicyProviderTest, PolicyDomainHandling) {
  // Initialize the provider
  provider_.Init(&schema_registry_);

  // Fire the observer event to trigger policy loading
  provider_.OnBravePoliciesReady();

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

TEST_F(BraveBrowserPolicyProviderTest, OnBrowserPolicyChanged) {
  // Initialize the provider
  provider_.Init(&schema_registry_);

  // Fire the observer event to trigger policy loading
  provider_.OnBravePoliciesReady();

  // Call OnBrowserPolicyChanged
  provider_.OnBrowserPolicyChanged("test.browser.pref");

  // Provider should still report policies as loaded after refresh
  EXPECT_TRUE(
      provider_.IsFirstPolicyLoadComplete(policy::POLICY_DOMAIN_CHROME));
}

}  // namespace brave_policy
