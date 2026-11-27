/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/decentralized_dns/content/decentralized_dns_navigation_throttle.h"

#include "base/memory/raw_ptr.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/prefs/testing_pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/mock_navigation_throttle_registry.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_manager.h"
#endif

namespace {

constexpr char kTestProfileName[] = "TestProfile";
constexpr char kLocale[] = "en-US";

}  // namespace

namespace decentralized_dns {

// Guest and Tor profiles are chrome/ Profile types, so these cases stay here.
// The profile-agnostic cases live in
// components/decentralized_dns/content/
// decentralized_dns_navigation_throttle_unittest.cc.
class DecentralizedDnsNavigationThrottleProfileTest : public testing::Test {
 public:
  DecentralizedDnsNavigationThrottleProfileTest()
      : profile_manager_(TestingBrowserProcess::GetGlobal()) {}
  ~DecentralizedDnsNavigationThrottleProfileTest() override = default;

  void SetUp() override {
    ASSERT_TRUE(profile_manager_.SetUp());
    profile_ = profile_manager_.CreateTestingProfile(kTestProfileName);
  }

  PrefService* user_prefs() { return user_prefs::UserPrefs::Get(profile_); }
  PrefService* local_state() {
    return TestingBrowserProcess::GetGlobal()->GetTestingLocalState();
  }

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
  raw_ptr<TestingProfile> profile_ = nullptr;
};

TEST_F(DecentralizedDnsNavigationThrottleProfileTest, NotInstantiatedInGuest) {
  auto* guest_profile = CreateGuestProfile();
  auto guest_web_contents =
      content::WebContentsTester::CreateTestWebContents(guest_profile, nullptr);
  content::MockNavigationHandle guest_test_handle(guest_web_contents.get());
  content::MockNavigationThrottleRegistry guest_registry(
      &guest_test_handle,
      content::MockNavigationThrottleRegistry::RegistrationMode::kHold);
  DecentralizedDnsNavigationThrottle::MaybeCreateAndAdd(
      guest_registry, user_prefs(), local_state(), locale());
  EXPECT_TRUE(guest_registry.throttles().empty());
}

#if BUILDFLAG(ENABLE_TOR)
TEST_F(DecentralizedDnsNavigationThrottleProfileTest, NotInstantiatedInTor) {
  Profile* tor_profile =
      TorProfileManager::GetInstance().GetTorProfile(profile());
  ASSERT_TRUE(tor_profile->IsTor());
  ASSERT_TRUE(tor_profile->IsOffTheRecord());

  auto tor_web_contents =
      content::WebContentsTester::CreateTestWebContents(tor_profile, nullptr);
  content::MockNavigationHandle tor_test_handle(tor_web_contents.get());
  content::MockNavigationThrottleRegistry tor_registry(
      &tor_test_handle,
      content::MockNavigationThrottleRegistry::RegistrationMode::kHold);
  DecentralizedDnsNavigationThrottle::MaybeCreateAndAdd(
      tor_registry, user_prefs(), local_state(), locale());
  EXPECT_TRUE(tor_registry.throttles().empty());
}
#endif

}  // namespace decentralized_dns
