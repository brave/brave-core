/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/decentralized_dns/content/decentralized_dns_navigation_throttle.h"

#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_manager.h"
#endif

namespace {

constexpr char kTestProfileName[] = "TestProfile";
constexpr char kExampleURL[] = "https://example.com";
constexpr char kLocale[] = "en-US";

}  // namespace

namespace decentralized_dns {

class DecentralizedDnsNavigationThrottleTest : public testing::Test {
 public:
  DecentralizedDnsNavigationThrottleTest()
      : profile_manager_(TestingBrowserProcess::GetGlobal()) {}
  ~DecentralizedDnsNavigationThrottleTest() override = default;

  void SetUp() override {
    ASSERT_TRUE(profile_manager_.SetUp());
    profile_ = profile_manager_.CreateTestingProfile(kTestProfileName);
    local_state_ = profile_manager_.local_state();
    web_contents_ =
        content::WebContentsTester::CreateTestWebContents(profile_, nullptr);
  }

  void TearDown() override { web_contents_.reset(); }

  PrefService* user_prefs() { return user_prefs::UserPrefs::Get(profile_); }
  PrefService* local_state() { return local_state_->Get(); }
  content::WebContents* web_contents() { return web_contents_.get(); }

  // Helper that creates simple test guest profile.
  Profile* CreateGuestProfile() {
    return profile_manager_.CreateGuestProfile()->GetPrimaryOTRProfile(
        /*create_if_needed=*/true);
  }

  TestingProfile* profile() { return profile_; }

  std::string locale() { return kLocale; }

 private:
  content::BrowserTaskEnvironment task_environment_;
  content::RenderViewHostTestEnabler test_render_host_factories_;
  TestingProfileManager profile_manager_;
  std::unique_ptr<content::WebContents> web_contents_;
  std::string locale_;
  raw_ptr<ScopedTestingLocalState> local_state_ = nullptr;
  raw_ptr<TestingProfile> profile_ = nullptr;
};

TEST_F(DecentralizedDnsNavigationThrottleTest, Instantiation) {
  content::MockNavigationHandle test_handle(web_contents());
  auto throttle = DecentralizedDnsNavigationThrottle::MaybeCreateThrottleFor(
      &test_handle, user_prefs(), local_state(), locale());
  EXPECT_TRUE(throttle != nullptr);

  // Disable in OTR profile.
  auto otr_web_contents = content::WebContentsTester::CreateTestWebContents(
      profile()->GetPrimaryOTRProfile(/*create_if_needed=*/true), nullptr);
  content::MockNavigationHandle otr_test_handle(otr_web_contents.get());
  auto throttle_in_otr =
      DecentralizedDnsNavigationThrottle::MaybeCreateThrottleFor(
          &otr_test_handle, user_prefs(), local_state(), locale());
  EXPECT_EQ(throttle_in_otr, nullptr);

  // Disable in guest profiles.
  auto* guest_profile = CreateGuestProfile();
  auto guest_web_contents =
      content::WebContentsTester::CreateTestWebContents(guest_profile, nullptr);
  content::MockNavigationHandle guest_test_handle(guest_web_contents.get());
  auto throttle_in_guest =
      DecentralizedDnsNavigationThrottle::MaybeCreateThrottleFor(
          &guest_test_handle, user_prefs(), local_state(), locale());
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
          &tor_test_handle, user_prefs(), local_state(), locale());
  EXPECT_EQ(throttle_in_tor, nullptr);
}
#endif

class DecentralizedDnsNavigationThrottleSubframeTest
    : public content::RenderViewHostTestHarness {
 public:
  DecentralizedDnsNavigationThrottleSubframeTest()
      : content::RenderViewHostTestHarness(
            base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        local_state_(TestingBrowserProcess::GetGlobal()) {}
  ~DecentralizedDnsNavigationThrottleSubframeTest() override = default;

  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();

    content::RenderFrameHostTester::For(main_rfh())
        ->InitializeRenderFrameIfNeeded();
    subframe_ = content::RenderFrameHostTester::For(main_rfh())
                    ->AppendChild("subframe");
  }

  PrefService* user_prefs() { return &prefs_; }
  PrefService* local_state() { return local_state_.Get(); }
  content::RenderFrameHost* subframe() { return subframe_; }
  std::string locale() { return kLocale; }

 private:
  raw_ptr<content::RenderFrameHost> subframe_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  ScopedTestingLocalState local_state_;
};

TEST_F(DecentralizedDnsNavigationThrottleSubframeTest, Subframe) {
  // Throttle is created for main frame.
  {
    content::MockNavigationHandle handle(GURL(kExampleURL), main_rfh());
    auto throttle = DecentralizedDnsNavigationThrottle::MaybeCreateThrottleFor(
        &handle, user_prefs(), local_state(), locale());
    EXPECT_NE(throttle, nullptr);
  }
  // Throttle is not created for subframe.
  {
    content::MockNavigationHandle handle(GURL(kExampleURL), subframe());
    auto throttle = DecentralizedDnsNavigationThrottle::MaybeCreateThrottleFor(
        &handle, user_prefs(), local_state(), locale());
    EXPECT_EQ(throttle, nullptr);
  }
}

}  // namespace decentralized_dns
