/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_service.h"

#include "base/files/file_path.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_origin/brave_origin_service_factory.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/brave_origin/features.h"
#include "brave/components/brave_policy/static_simple_policy_handler.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/policy/profile_policy_connector.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_test_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"

namespace policy {

class BraveOriginServicePolicyTest : public InProcessBrowserTest {
 public:
  BraveOriginServicePolicyTest() {
    feature_list_.InitAndEnableFeature(brave_origin::features::kBraveOrigin);
  }
  ~BraveOriginServicePolicyTest() override = default;

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveOriginServicePolicyTest,
                       AIChatPolicyCachedAfterFirstLoad) {
  auto* profile1 = browser()->profile();
  auto* prefs1 = profile1->GetPrefs();

  // Create a second profile
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  base::FilePath profile_path =
      profile_manager->GenerateNextProfileDirectoryPath();
  Profile& profile2 =
      profiles::testing::CreateProfileSync(profile_manager, profile_path);
  auto* prefs2 = profile2.GetPrefs();

  auto* origin_service1 =
      brave_origin::BraveOriginServiceFactory::GetForProfile(profile1);
  ASSERT_TRUE(origin_service1);

  auto* origin_service2 =
      brave_origin::BraveOriginServiceFactory::GetForProfile(&profile2);
  ASSERT_TRUE(origin_service2);

  // Enable cache bypass initially so we can set up initial state
  StaticSimplePolicyHandler::SetCacheBypassForTesting(true);

  // Enable AIChat for profile 1
  EXPECT_TRUE(origin_service1->SetPolicyValue(key::kBraveAIChatEnabled, true));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(prefs1->GetBoolean(ai_chat::prefs::kEnabledByPolicy));

  // Disable AIChat for profile 2
  EXPECT_TRUE(origin_service2->SetPolicyValue(key::kBraveAIChatEnabled, true));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(origin_service2->SetPolicyValue(key::kBraveAIChatEnabled, false));
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(prefs1->GetBoolean(ai_chat::prefs::kEnabledByPolicy));
  EXPECT_FALSE(prefs2->GetBoolean(ai_chat::prefs::kEnabledByPolicy));

  // Disable cache bypass - this emulates the "first load"
  StaticSimplePolicyHandler::SetCacheBypassForTesting(false);

  // Invert the policy values to test the caching
  EXPECT_TRUE(origin_service1->SetPolicyValue(key::kBraveAIChatEnabled, false));
  EXPECT_TRUE(origin_service2->SetPolicyValue(key::kBraveAIChatEnabled, true));
  base::RunLoop().RunUntilIdle();

  // This pref will be true because the value is cached from the "first load"
  EXPECT_TRUE(prefs1->GetBoolean(ai_chat::prefs::kEnabledByPolicy));
  // The following expectation will fail (it will return true), because the
  // policy handler caches a single value for all profile policy services,
  // demonstrating the limitation of the static policy handler implementation.
  EXPECT_FALSE(prefs2->GetBoolean(ai_chat::prefs::kEnabledByPolicy))
      << "This expectation will fail";
}

}  // namespace policy
