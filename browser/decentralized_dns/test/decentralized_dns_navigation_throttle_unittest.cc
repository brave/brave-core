/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/decentralized_dns/decentralized_dns_navigation_throttle.h"

#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/decentralized_dns/features.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_manager.h"
#endif

constexpr char kTestProfileName[] = "TestProfile";

namespace decentralized_dns {

class DecentralizedDnsNavigationThrottleTest : public testing::Test {
 public:
  DecentralizedDnsNavigationThrottleTest()
      : profile_manager_(TestingBrowserProcess::GetGlobal()) {}
  ~DecentralizedDnsNavigationThrottleTest() override = default;

  void SetUp() override {
    feature_list_.InitAndEnableFeature(features::kDecentralizedDns);

    ASSERT_TRUE(profile_manager_.SetUp());
    profile_ = profile_manager_.CreateTestingProfile(kTestProfileName);
    local_state_ = profile_manager_.local_state();
    web_contents_ =
        content::WebContentsTester::CreateTestWebContents(profile_, nullptr);
    locale_ = "en-US";
  }

  void TearDown() override { web_contents_.reset(); }

  PrefService* local_state() { return local_state_->Get(); }
  content::WebContents* web_contents() { return web_contents_.get(); }

  // Helper that creates simple test guest profile.
  Profile* CreateGuestProfile() {
    return profile_manager_.CreateGuestProfile()->GetPrimaryOTRProfile(
        /*create_if_needed=*/true);
  }

  TestingProfile* profile() { return profile_; }

  const std::string& locale() { return locale_; }

 private:
  content::BrowserTaskEnvironment task_environment_;
  content::RenderViewHostTestEnabler test_render_host_factories_;
  raw_ptr<ScopedTestingLocalState> local_state_ = nullptr;
  TestingProfileManager profile_manager_;
  raw_ptr<TestingProfile> profile_ = nullptr;
  base::test::ScopedFeatureList feature_list_;
  std::unique_ptr<content::WebContents> web_contents_;
  std::string locale_;
};

TEST_F(DecentralizedDnsNavigationThrottleTest, Instantiation) {
  content::MockNavigationHandle test_handle(web_contents());
  auto throttle = DecentralizedDnsNavigationThrottle::MaybeCreateThrottleFor(
      &test_handle, local_state(), locale());
  EXPECT_TRUE(throttle != nullptr);

  // Disable in OTR profile.
  auto otr_web_contents = content::WebContentsTester::CreateTestWebContents(
      profile()->GetPrimaryOTRProfile(/*create_if_needed=*/true), nullptr);
  content::MockNavigationHandle otr_test_handle(otr_web_contents.get());
  auto throttle_in_otr =
      DecentralizedDnsNavigationThrottle::MaybeCreateThrottleFor(
          &otr_test_handle, local_state(), locale());
  EXPECT_EQ(throttle_in_otr, nullptr);

  // Disable in guest profiles.
  auto* guest_profile = CreateGuestProfile();
  auto guest_web_contents =
      content::WebContentsTester::CreateTestWebContents(guest_profile, nullptr);
  content::MockNavigationHandle guest_test_handle(guest_web_contents.get());
  auto throttle_in_guest =
      DecentralizedDnsNavigationThrottle::MaybeCreateThrottleFor(
          &guest_test_handle, local_state(), locale());
  EXPECT_EQ(throttle_in_guest, nullptr);
}

#if BUILDFLAG(ENABLE_TOR)
TEST_F(DecentralizedDnsNavigationThrottleTest, NotInstantiatedInTor) {
  Profile* tor_profile =
      TorProfileManager::GetInstance().GetTorProfile(profile());
  ASSERT_TRUE(tor_profile->IsTor());
  ASSERT_TRUE(tor_profile->IsOffTheRecord());

  auto tor_web_contents =
      content::WebContentsTester::CreateTestWebContents(tor_profile, nullptr);
  content::MockNavigationHandle tor_test_handle(tor_web_contents.get());
  auto throttle_in_tor =
      DecentralizedDnsNavigationThrottle::MaybeCreateThrottleFor(
          &tor_test_handle, local_state(), locale());
  EXPECT_EQ(throttle_in_tor, nullptr);
}
#endif

}  // namespace decentralized_dns
