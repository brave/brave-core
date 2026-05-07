/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_policy/brave_profile_policy_provider.h"

#include <memory>

#include "base/files/file_path.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_origin/brave_origin_policy_info.h"
#include "brave/components/brave_origin/brave_origin_policy_manager.h"
#include "brave/components/brave_origin/pref_names.h"
#include "brave/components/brave_policy/ad_block_only_mode/ad_block_only_mode_policy_manager.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/policy/core/common/configuration_policy_provider.h"
#include "components/policy/core/common/policy_bundle.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/core/common/schema_registry.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_policy {

class BraveProfilePolicyProviderTest : public ::testing::Test {
 public:
  BraveProfilePolicyProviderTest() = default;
  ~BraveProfilePolicyProviderTest() override = default;

  void SetUp() override {
    // Register prefs needed by both managers so their `Init` calls succeed
    // and they report `IsInitialized() == true`. Without this the
    // `BravePolicyManagerRegistry::AllInitialized()` gate inside
    // `BraveProfilePolicyProvider::OnBravePoliciesReady` would early-return
    // and the provider would never call `RefreshPolicies`.
    pref_service_.registry()->RegisterDictionaryPref(
        brave_origin::kBraveOriginPolicies);
    pref_service_.registry()->RegisterBooleanPref(
        brave_origin::kOriginPurchaseValidated, false);
    pref_service_.registry()->RegisterBooleanPref(
        brave_shields::prefs::kAdBlockOnlyModeEnabled, false);

    brave_origin::BraveOriginPolicyManager::GetInstance()->Init(
        brave_origin::BraveOriginPolicyMap(),
        brave_origin::BraveOriginPolicyMap(), &pref_service_);
    AdBlockOnlyModePolicyManager::GetInstance()->Init(&pref_service_);
  }

  void TearDown() override {
    if (provider_.IsInitializationComplete(policy::POLICY_DOMAIN_CHROME)) {
      provider_.Shutdown();
    }
    AdBlockOnlyModePolicyManager::GetInstance()->Shutdown();
    brave_origin::BraveOriginPolicyManager::GetInstance()->Shutdown();
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
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
  provider_.OnBravePoliciesReady();
  provider_.SetProfileID("test-profile-id");

  // Now policies should be loaded
  EXPECT_TRUE(
      provider_.IsFirstPolicyLoadComplete(policy::POLICY_DOMAIN_CHROME));
}

TEST_F(BraveProfilePolicyProviderTest, EmptyPolicyBundle) {
  // Initialize the provider
  provider_.Init(&schema_registry_);

  // Fire the observer event and set profile ID to trigger policy loading
  provider_.OnBravePoliciesReady();
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
  provider_.OnBravePoliciesReady();
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
  provider_.OnBravePoliciesReady();
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
  provider_.OnBravePoliciesReady();
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
  provider_.OnBravePoliciesReady();
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
  provider_.OnBravePoliciesReady();
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
  provider_.OnBravePoliciesReady();

  // Call OnProfilePolicyChanged (should not crash or affect anything)
  provider_.OnProfilePolicyChanged("test.pref.policy", "some-profile-id");

  // Provider should still report no policies loaded since no profile ID set
  EXPECT_FALSE(
      provider_.IsFirstPolicyLoadComplete(policy::POLICY_DOMAIN_CHROME));
}

// ---- Pending profile-path stash ----

class BraveProfilePolicyProviderStashTest : public ::testing::Test {
 protected:
  void TearDown() override {
    // Defensive: clear the stash so tests don't leak state between cases.
    BraveProfilePolicyProvider::TakePendingProfilePath();
  }
};

TEST_F(BraveProfilePolicyProviderStashTest, TakeWithoutSetReturnsEmpty) {
  EXPECT_TRUE(BraveProfilePolicyProvider::TakePendingProfilePath().empty());
}

TEST_F(BraveProfilePolicyProviderStashTest, SetThenTakeReturnsSamePath) {
  const base::FilePath path(FILE_PATH_LITERAL("/tmp/profile/Default"));
  BraveProfilePolicyProvider::SetPendingProfilePath(path);
  EXPECT_EQ(path, BraveProfilePolicyProvider::TakePendingProfilePath());
}

TEST_F(BraveProfilePolicyProviderStashTest, TakeClearsStash) {
  BraveProfilePolicyProvider::SetPendingProfilePath(
      base::FilePath(FILE_PATH_LITERAL("/tmp/profile/Default")));
  EXPECT_FALSE(BraveProfilePolicyProvider::TakePendingProfilePath().empty());
  // Subsequent Take should return empty.
  EXPECT_TRUE(BraveProfilePolicyProvider::TakePendingProfilePath().empty());
}

TEST_F(BraveProfilePolicyProviderStashTest, RepeatedSetOnlyKeepsLatest) {
  BraveProfilePolicyProvider::SetPendingProfilePath(
      base::FilePath(FILE_PATH_LITERAL("/tmp/profile/A")));
  BraveProfilePolicyProvider::SetPendingProfilePath(
      base::FilePath(FILE_PATH_LITERAL("/tmp/profile/B")));
  // Latest set wins; no leak from prior Set.
  EXPECT_EQ(base::FilePath(FILE_PATH_LITERAL("/tmp/profile/B")),
            BraveProfilePolicyProvider::TakePendingProfilePath());
  // Only one path was stashed; subsequent Take is empty.
  EXPECT_TRUE(BraveProfilePolicyProvider::TakePendingProfilePath().empty());
}

// ---- SetProfileID idempotency ----

namespace {

// Counts how many times the provider notifies observers via UpdatePolicy.
class UpdateCountingObserver
    : public policy::ConfigurationPolicyProvider::Observer {
 public:
  void OnUpdatePolicy(
      policy::ConfigurationPolicyProvider* /*provider*/) override {
    ++count_;
  }
  int count() const { return count_; }

 private:
  int count_ = 0;
};

}  // namespace

TEST_F(BraveProfilePolicyProviderTest, SetProfileID_SameIdIsIdempotent) {
  provider_.Init(&schema_registry_);
  UpdateCountingObserver observer;
  provider_.AddObserver(&observer);

  // Make policies_ready_ true so SetProfileID would otherwise fire
  // RefreshPolicies.
  provider_.OnBravePoliciesReady();

  // First SetProfileID triggers a RefreshPolicies → one observer notification.
  provider_.SetProfileID("test-profile-id");
  const int after_first = observer.count();
  EXPECT_GT(after_first, 0);

  // Calling SetProfileID with the same id should be a no-op — no new
  // notifications.
  provider_.SetProfileID("test-profile-id");
  EXPECT_EQ(after_first, observer.count());

  // Calling with a different id does fire RefreshPolicies again.
  provider_.SetProfileID("different-id");
  EXPECT_GT(observer.count(), after_first);

  provider_.RemoveObserver(&observer);
}

}  // namespace brave_policy
