/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_origin/brave_origin_service_factory.h"

#include "base/containers/map_util.h"
#include "brave/browser/policy/brave_simple_policy_map.h"
#include "brave/components/brave_origin/brave_origin_policy_info.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/p3a/pref_names.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/policy/core/common/policy_details.h"
#include "components/policy/policy_constants.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/pref_names.h"
#endif

namespace brave_origin {

TEST(BraveOriginServiceFactoryTest,
     GetBrowserPolicyDefinitions_ContainsExpectedBrowserPolicies) {
  auto browser_policy_definitions =
      BraveOriginServiceFactory::GetBrowserPolicyDefinitions();

  // Test that P3A policy is correctly built (browser-level)
  const auto* p3a_info = base::FindOrNull(browser_policy_definitions,
                                          policy::key::kBraveP3AEnabled);
  ASSERT_NE(p3a_info, nullptr);
  EXPECT_EQ(p3a_info->pref_name, p3a::kP3AEnabled);
  EXPECT_EQ(p3a_info->default_value, false);
  EXPECT_EQ(p3a_info->user_settable, true);
  EXPECT_EQ(p3a_info->brave_origin_pref_key, p3a::kP3AEnabled);

  // Test that Stats reporting policy is correctly built (browser-level)
  const auto* stats_info = base::FindOrNull(
      browser_policy_definitions, policy::key::kBraveStatsPingEnabled);
  ASSERT_NE(stats_info, nullptr);
  EXPECT_EQ(stats_info->pref_name, kStatsReportingEnabled);
  EXPECT_EQ(stats_info->default_value, false);
  EXPECT_EQ(stats_info->user_settable, true);
  EXPECT_EQ(stats_info->brave_origin_pref_key, kStatsReportingEnabled);

#if BUILDFLAG(ENABLE_TOR)
  // Test that Tor disabled policy is correctly built (browser-level)
  const auto* tor_info =
      base::FindOrNull(browser_policy_definitions, policy::key::kTorDisabled);
  ASSERT_NE(tor_info, nullptr);
  EXPECT_EQ(tor_info->pref_name, tor::prefs::kTorDisabled);
  EXPECT_EQ(tor_info->default_value,
            true);  // This is a "disabled" pref, so default is true
  EXPECT_EQ(tor_info->user_settable, false);
  EXPECT_EQ(tor_info->brave_origin_pref_key, tor::prefs::kTorDisabled);
#endif

  // Test that profile-level policies are NOT in browser definitions
  EXPECT_FALSE(
      browser_policy_definitions.contains(policy::key::kBraveRewardsDisabled))
      << "Profile-level policy should not be in browser definitions";
}

TEST(BraveOriginServiceFactoryTest,
     GetProfilePolicyDefinitions_ContainsExpectedProfilePolicies) {
  auto profile_policy_definitions =
      BraveOriginServiceFactory::GetProfilePolicyDefinitions();

  // Test that Brave Rewards disabled policy is correctly built (profile-level)
  const auto* rewards_info = base::FindOrNull(
      profile_policy_definitions, policy::key::kBraveRewardsDisabled);
  ASSERT_NE(rewards_info, nullptr);
  EXPECT_EQ(rewards_info->pref_name, brave_rewards::prefs::kDisabledByPolicy);
  EXPECT_EQ(rewards_info->default_value,
            true);  // This is a "disabled" pref, so default is true
  EXPECT_EQ(rewards_info->user_settable, false);
  EXPECT_EQ(rewards_info->brave_origin_pref_key,
            brave_rewards::prefs::kDisabledByPolicy);

  // Test that browser-level policies are NOT in profile definitions
  EXPECT_FALSE(
      profile_policy_definitions.contains(policy::key::kBraveP3AEnabled))
      << "Browser-level policy should not be in profile definitions";
  EXPECT_FALSE(
      profile_policy_definitions.contains(policy::key::kBraveStatsPingEnabled))
      << "Browser-level policy should not be in profile definitions";
}

TEST(BraveOriginServiceFactoryTest,
     GetBrowserPolicyDefinitions_ExcludesPoliciesNotInMetadata) {
  auto policy_definitions =
      BraveOriginServiceFactory::GetBrowserPolicyDefinitions();

  // Test that policies in kBraveSimplePolicyMap but NOT in kBraveOriginMetadata
  // are excluded kBraveShieldsDisabledForUrls is in kBraveSimplePolicyMap but
  // not in kBraveOriginMetadata
  EXPECT_FALSE(
      policy_definitions.contains(policy::key::kBraveShieldsDisabledForUrls))
      << "kManagedBraveShieldsDisabledForUrls should not be in policy "
         "definitions";

  // kBraveShieldsEnabledForUrls is also in kBraveSimplePolicyMap but not in
  // kBraveOriginMetadata
  EXPECT_FALSE(
      policy_definitions.contains(policy::key::kBraveShieldsEnabledForUrls))
      << "kManagedBraveShieldsEnabledForUrls should not be in policy "
         "definitions";
}

TEST(BraveOriginServiceFactoryTest,
     GetPolicyDefinitions_OnlyContainsMetadataPolicies) {
  auto browser_policy_definitions =
      BraveOriginServiceFactory::GetBrowserPolicyDefinitions();
  auto profile_policy_definitions =
      BraveOriginServiceFactory::GetProfilePolicyDefinitions();

  // Test browser policy definitions
  for (const auto& [policy_key, policy_info] : browser_policy_definitions) {
    EXPECT_FALSE(policy_key.empty())
        << "Browser policy key should not be empty";
    EXPECT_FALSE(policy_info.pref_name.empty())
        << "Browser policy info pref name should not be empty";
    EXPECT_FALSE(policy_info.brave_origin_pref_key.empty())
        << "Browser BraveOrigin pref key should not be empty";
    EXPECT_EQ(policy_info.brave_origin_pref_key, policy_info.pref_name);
  }

  // Test profile policy definitions
  for (const auto& [policy_key, policy_info] : profile_policy_definitions) {
    EXPECT_FALSE(policy_key.empty())
        << "Profile policy key should not be empty";
    EXPECT_FALSE(policy_info.pref_name.empty())
        << "Profile policy info pref name should not be empty";
    EXPECT_FALSE(policy_info.brave_origin_pref_key.empty())
        << "Profile BraveOrigin pref key should not be empty";
    EXPECT_EQ(policy_info.brave_origin_pref_key, policy_info.pref_name);
  }

  // Verify that we have browser-level policies (P3A, Stats are always
  // available)
  EXPECT_GE(browser_policy_definitions.size(), 2u)
      << "Should have at least P3A and Stats browser policies";

  // Verify that we have profile-level policies
  EXPECT_GT(profile_policy_definitions.size(), 0u)
      << "Should have at least some profile policies";
}

// Verifies that every policy claimed by Brave Origin is actually enforceable
// on the current build platform — i.e. its YAML lists the platform under
// `supported_on:`, not `future_on:`.
//
// Future policies are dropped by `IsBlockedFuturePolicy`
// (configuration_policy_handler_list.cc) on Stable and Beta channels, so a
// future policy claimed by Brave Origin silently fails to manage its pref for
// end users — even though debug/Nightly builds (where future policies are
// allowed) make it look like it works. Direct admin policy via
// `kBraveSimplePolicyMap` is allowed to be future on a given platform; this
// invariant is specifically about Brave Origin's user-facing toggles.
TEST(BraveOriginServiceFactoryTest, OriginPolicies_NotFutureOnThisPlatform) {
  auto check = [](const BraveOriginPolicyMap& definitions) {
    for (const auto& [policy_key, info] : definitions) {
      const policy::PolicyDetails* details =
          policy::GetChromePolicyDetails(policy_key);
      ASSERT_NE(details, nullptr)
          << "Unknown policy in Brave Origin definitions: " << policy_key;
      EXPECT_FALSE(details->is_future)
          << policy_key
          << " is in Brave Origin metadata but its YAML lists the current "
             "build platform under future_on:. On Stable/Beta channels the "
             "policy is dropped by IsBlockedFuturePolicy before reaching the "
             "SimplePolicyHandler, so when an Origin user toggles this "
             "feature the pref never becomes managed. To fix, pick one:\n"
             "\n"
             "  (a) If the platform's runtime support is shipped, move it "
             "from future_on: to supported_on: in the policy YAML at\n"
             "      brave/components/policy/resources/templates/"
             "policy_definitions/BraveSoftware/"
          << policy_key
          << ".yaml\n"
             "      (e.g. add `- android:<milestone>-` to supported_on:, "
             "drop the future_on: block).\n"
             "\n"
             "  (b) If the platform's runtime support is NOT shipped, "
             "remove (or platform-gate) this policy's entry in the matching "
             "metadata map (kBraveOriginBrowserMetadata or "
             "kBraveOriginProfileMetadata) at\n"
             "      brave/browser/brave_origin/"
             "brave_origin_service_factory.cc\n"
             "      so that Brave Origin no longer claims to enforce this "
             "policy on the current platform.\n"
             "\n"
             "Either fix makes the YAML, the Brave Origin metadata, and the "
             "runtime tell a consistent story.";
    }
  };
  check(BraveOriginServiceFactory::GetBrowserPolicyDefinitions());
  check(BraveOriginServiceFactory::GetProfilePolicyDefinitions());
}

// Test fixture for profile-related tests
class BraveOriginServiceFactoryProfileTest : public ::testing::Test {
 protected:
  void SetUp() override { ASSERT_TRUE(profile_manager_.SetUp()); }

  content::BrowserTaskEnvironment task_environment_;
  TestingProfileManager profile_manager_{TestingBrowserProcess::GetGlobal()};
};

TEST_F(BraveOriginServiceFactoryProfileTest, NoServiceForGuestProfile) {
  // Create a guest profile
  auto* guest_profile = profile_manager_.CreateGuestProfile();
  ASSERT_NE(guest_profile, nullptr);

  // Verify that BraveOriginService is not created for guest profiles
  auto* service = BraveOriginServiceFactory::GetForProfile(guest_profile);
  EXPECT_EQ(service, nullptr);
}

}  // namespace brave_origin
