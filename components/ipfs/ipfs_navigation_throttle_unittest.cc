/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_navigation_throttle.h"

#include <memory>
#include <string>
#include <vector>

#include "base/test/bind_test_util.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/browser/profiles/brave_unittest_profile_manager.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/components/ipfs/features.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/pref_names.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/test_utils.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using content::NavigationThrottle;

namespace {

const GURL& GetIPFSURL() {
  static const GURL ipfs_url(
      "http://localhost:48080/ipfs/"
      "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/"
      "Vincent_van_Gogh.html");  // NOLINT
  return ipfs_url;
}

const GURL& GetIPNSURL() {
  static const GURL ipns_url(
      "http://localhost:48080/ipns/tr.wikipedia-on-ipfs.org/wiki/"
      "Anasayfa.html");  // NOLINT
  return ipns_url;
}

}  // namespace

namespace ipfs {

class IpfsNavigationThrottleUnitTest : public testing::Test {
 public:
  IpfsNavigationThrottleUnitTest()
      : local_state_(TestingBrowserProcess::GetGlobal()) {}
  ~IpfsNavigationThrottleUnitTest() override = default;

  void SetUp() override {
    feature_list_.InitAndEnableFeature(ipfs::features::kIpfsFeature);
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    TestingBrowserProcess::GetGlobal()->SetProfileManager(
        new BraveUnittestProfileManager(temp_dir_.GetPath()));
    ProfileManager* profile_manager = g_browser_process->profile_manager();
    ASSERT_TRUE(profile_manager);

    profile_ = profile_manager->GetProfile(
        temp_dir_.GetPath().AppendASCII(TestingProfile::kTestUserProfileDir));
    web_contents_ =
        content::WebContentsTester::CreateTestWebContents(profile_, nullptr);
    ipfs_service_ = IpfsServiceFactory::GetForContext(profile_);
    locale_ = "en-US";
  }

  void TearDown() override {
    web_contents_.reset();
    TestingBrowserProcess::GetGlobal()->SetProfileManager(nullptr);
  }

  content::WebContents* web_contents() { return web_contents_.get(); }

  IpfsService* ipfs_service() { return ipfs_service_; }

  base::ScopedTempDir* temp_dir() { return &temp_dir_; }

  // Helper that creates simple test guest profile.
  std::unique_ptr<TestingProfile> CreateGuestProfile() {
    TestingProfile::Builder profile_builder;
    profile_builder.SetGuestSession();
    return profile_builder.Build();
  }

  Profile* profile() { return profile_; }

  const std::string& locale() { return locale_; }

 private:
  content::BrowserTaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  ScopedTestingLocalState local_state_;
  content::RenderViewHostTestEnabler test_render_host_factories_;
  std::unique_ptr<content::WebContents> web_contents_;
  IpfsService* ipfs_service_;
  Profile* profile_;
  base::test::ScopedFeatureList feature_list_;
  std::string locale_;

  DISALLOW_COPY_AND_ASSIGN(IpfsNavigationThrottleUnitTest);
};

TEST_F(IpfsNavigationThrottleUnitTest, DeferUntilIpfsProcessLaunched) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod, static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));

  auto peers = std::vector<std::string>{
      "/ip4/101.101.101.101/tcp/4001/p2p/"
      "QmaCpDMGvV2BGHeYERUEnRQAwe3N8SzbUtfsmvsqQLuvuJ"};

  ipfs_service()->SetSkipGetConnectedPeersCallbackForTest(true);

  content::MockNavigationHandle test_handle(web_contents());
  test_handle.set_url(GetIPFSURL());
  auto throttle = IpfsNavigationThrottle::MaybeCreateThrottleFor(
      &test_handle, ipfs_service(), brave::IsRegularProfile(profile()),
      locale());
  ASSERT_TRUE(throttle != nullptr);
  bool was_navigation_resumed = false;
  throttle->set_resume_callback_for_testing(
      base::BindLambdaForTesting([&]() { was_navigation_resumed = true; }));
  EXPECT_EQ(NavigationThrottle::DEFER, throttle->WillStartRequest().action())
      << GetIPFSURL();
  ipfs_service()->SetIpfsLaunchedForTest(true);
  ipfs_service()->RunLaunchDaemonCallbackForTest(true);
  EXPECT_TRUE(was_navigation_resumed);

  was_navigation_resumed = false;
  EXPECT_EQ(NavigationThrottle::DEFER, throttle->WillStartRequest().action())
      << GetIPFSURL();
  throttle->OnGetConnectedPeers(true, peers);
  EXPECT_TRUE(was_navigation_resumed);

  ipfs_service()->SetIpfsLaunchedForTest(false);

  was_navigation_resumed = false;
  test_handle.set_url(GetIPNSURL());
  EXPECT_EQ(NavigationThrottle::DEFER, throttle->WillStartRequest().action())
      << GetIPNSURL();
  ipfs_service()->SetIpfsLaunchedForTest(true);
  ipfs_service()->RunLaunchDaemonCallbackForTest(true);
  EXPECT_TRUE(was_navigation_resumed);

  was_navigation_resumed = false;
  EXPECT_EQ(NavigationThrottle::DEFER, throttle->WillStartRequest().action())
      << GetIPNSURL();
  throttle->OnGetConnectedPeers(true, peers);
  EXPECT_TRUE(was_navigation_resumed);
}

TEST_F(IpfsNavigationThrottleUnitTest, ProceedForGatewayNodeMode) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_GATEWAY));

  content::MockNavigationHandle test_handle(web_contents());
  test_handle.set_url(GetIPFSURL());
  auto throttle = IpfsNavigationThrottle::MaybeCreateThrottleFor(
      &test_handle, ipfs_service(), brave::IsRegularProfile(profile()),
      locale());
  ASSERT_TRUE(throttle != nullptr);
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << GetIPFSURL();

  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_DISABLED));
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << GetIPFSURL();
}

TEST_F(IpfsNavigationThrottleUnitTest, ProceedForAskNodeMode) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod, static_cast<int>(IPFSResolveMethodTypes::IPFS_ASK));

  content::MockNavigationHandle test_handle(web_contents());
  test_handle.set_url(GetIPFSURL());
  auto throttle = IpfsNavigationThrottle::MaybeCreateThrottleFor(
      &test_handle, ipfs_service(), brave::IsRegularProfile(profile()),
      locale());
  ASSERT_TRUE(throttle != nullptr);
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << GetIPFSURL();

  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_DISABLED));
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << GetIPFSURL();
}

TEST_F(IpfsNavigationThrottleUnitTest, Instantiation) {
  content::MockNavigationHandle test_handle(web_contents());
  auto throttle = IpfsNavigationThrottle::MaybeCreateThrottleFor(
      &test_handle, ipfs_service(), brave::IsRegularProfile(profile()),
      locale());
  EXPECT_TRUE(throttle != nullptr);

  // Disable in OTR profile.
  auto otr_web_contents = content::WebContentsTester::CreateTestWebContents(
      profile()->GetPrimaryOTRProfile(), nullptr);
  content::MockNavigationHandle otr_test_handle(otr_web_contents.get());
  auto throttle_in_otr = IpfsNavigationThrottle::MaybeCreateThrottleFor(
      &otr_test_handle, ipfs_service(),
      brave::IsRegularProfile(profile()->GetPrimaryOTRProfile()), locale());
  EXPECT_EQ(throttle_in_otr, nullptr);

  // Disable in guest sessions.
  auto guest_profile = CreateGuestProfile();
  auto guest_web_contents = content::WebContentsTester::CreateTestWebContents(
      guest_profile.get(), nullptr);
  content::MockNavigationHandle guest_test_handle(guest_web_contents.get());
  auto throttle_in_guest = IpfsNavigationThrottle::MaybeCreateThrottleFor(
      &guest_test_handle, ipfs_service(),
      brave::IsRegularProfile(guest_profile.get()), locale());
  EXPECT_EQ(throttle_in_guest, nullptr);
}

#if BUILDFLAG(ENABLE_TOR)
TEST_F(IpfsNavigationThrottleUnitTest, NotInstantiatedInTor) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ASSERT_TRUE(profile_manager);

  Profile* tor_reg_profile =
      profile_manager->GetProfile(BraveProfileManager::GetTorProfilePath());
  ASSERT_EQ(brave::GetParentProfile(tor_reg_profile), profile());
  ASSERT_TRUE(brave::IsTorProfile(tor_reg_profile));
  ASSERT_FALSE(tor_reg_profile->IsOffTheRecord());

  auto tor_reg_web_contents = content::WebContentsTester::CreateTestWebContents(
      tor_reg_profile, nullptr);
  content::MockNavigationHandle tor_reg_test_handle(tor_reg_web_contents.get());
  auto throttle_in_tor_reg = IpfsNavigationThrottle::MaybeCreateThrottleFor(
      &tor_reg_test_handle, ipfs_service(),
      brave::IsRegularProfile(tor_reg_profile), locale());
  EXPECT_EQ(throttle_in_tor_reg, nullptr);

  auto tor_otr_web_contents = content::WebContentsTester::CreateTestWebContents(
      tor_reg_profile->GetPrimaryOTRProfile(), nullptr);
  content::MockNavigationHandle tor_otr_test_handle(tor_otr_web_contents.get());
  auto throttle_in_tor_otr = IpfsNavigationThrottle::MaybeCreateThrottleFor(
      &tor_otr_test_handle, ipfs_service(),
      brave::IsRegularProfile(tor_reg_profile), locale());
  EXPECT_EQ(throttle_in_tor_otr, nullptr);
}
#endif

}  // namespace ipfs
