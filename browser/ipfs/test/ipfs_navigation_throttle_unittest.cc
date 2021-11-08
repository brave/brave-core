/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_navigation_throttle.h"

#include <memory>
#include <string>
#include <vector>

#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/components/ipfs/features.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/keys/ipns_keys_manager.h"
#include "brave/components/ipfs/pref_names.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/test_utils.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_manager.h"
#endif

using content::NavigationThrottle;

namespace {

constexpr char kTestProfileName[] = "TestProfile";

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

const GURL& GetPublicGatewayURL() {
  static const GURL public_gw_url(
      "https://dweb.link/ipfs/QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR");
  return public_gw_url;
}

const GURL& GetNonIPFSURL() {
  static const GURL non_ipfs_url("http://github.com/ipfs/go-ipfs");
  return non_ipfs_url;
}

}  // namespace

namespace ipfs {

class IpfsNavigationThrottleUnitTest : public testing::Test {
 public:
  IpfsNavigationThrottleUnitTest() = default;
  IpfsNavigationThrottleUnitTest(const IpfsNavigationThrottleUnitTest&) =
      delete;
  IpfsNavigationThrottleUnitTest& operator=(
      const IpfsNavigationThrottleUnitTest&) = delete;
  ~IpfsNavigationThrottleUnitTest() override = default;

  void SetUp() override {
    feature_list_.InitAndEnableFeature(ipfs::features::kIpfsFeature);
    TestingBrowserProcess* browser_process = TestingBrowserProcess::GetGlobal();
    profile_manager_.reset(new TestingProfileManager(browser_process));
    ASSERT_TRUE(profile_manager_->SetUp());

    profile_ = profile_manager_->CreateTestingProfile(kTestProfileName);

    web_contents_ =
        content::WebContentsTester::CreateTestWebContents(profile_, nullptr);
    locale_ = "en-US";
  }

  void TearDown() override {
    web_contents_.reset();
    profile_ = nullptr;
    profile_manager_->DeleteTestingProfile(kTestProfileName);
  }

  content::WebContents* web_contents() { return web_contents_.get(); }

  std::unique_ptr<IpfsNavigationThrottle> CreateDeferredNavigation(
      IpfsService* service,
      const base::RepeatingClosure& resume_callback) {
    content::MockNavigationHandle test_handle(web_contents());
    test_handle.set_url(GetIPFSURL());
    std::unique_ptr<IpfsNavigationThrottle> throttle =
        IpfsNavigationThrottle::MaybeCreateThrottleFor(
            &test_handle, service, profile_->GetPrefs(), locale());
    throttle->set_resume_callback_for_testing(resume_callback);
    EXPECT_EQ(NavigationThrottle::DEFER, throttle->WillStartRequest().action())
        << GetIPFSURL();
    return throttle;
  }

  IpfsService* ipfs_service(content::BrowserContext* context) {
    return IpfsServiceFactory::GetForContext(context);
  }

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
  content::RenderViewHostTestEnabler test_render_host_factories_;
  std::unique_ptr<content::WebContents> web_contents_;
  Profile* profile_;
  std::unique_ptr<TestingProfileManager> profile_manager_;
  base::test::ScopedFeatureList feature_list_;
  std::string locale_;
};

TEST_F(IpfsNavigationThrottleUnitTest, DeferMultipleUntilIpfsProcessLaunched) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod, static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));

  auto* service = ipfs_service(profile());
  ASSERT_TRUE(service);
  service->SetSkipGetConnectedPeersCallbackForTest(true);
  service->GetIpnsKeysManager()->SetLoadCallbackForTest(base::DoNothing());
  bool was_navigation_resumed1 = false;
  auto throttle1 = CreateDeferredNavigation(
      service,
      base::BindLambdaForTesting([&]() { was_navigation_resumed1 = true; }));

  bool was_navigation_resumed2 = false;
  auto throttle2 = CreateDeferredNavigation(
      service,
      base::BindLambdaForTesting([&]() { was_navigation_resumed2 = true; }));

  bool was_navigation_resumed3 = false;
  auto throttle3 = CreateDeferredNavigation(
      service,
      base::BindLambdaForTesting([&]() { was_navigation_resumed3 = true; }));

  service->SetAllowIpfsLaunchForTest(true);
  service->RunLaunchDaemonCallbackForTest(true);
  EXPECT_FALSE(was_navigation_resumed1);
  EXPECT_FALSE(was_navigation_resumed2);
  EXPECT_FALSE(was_navigation_resumed3);
  auto peers = std::vector<std::string>{
      "/ip4/101.101.101.101/tcp/4001/p2p/"
      "QmaCpDMGvV2BGHeYERUEnRQAwe3N8SzbUtfsmvsqQLuvuJ"};
  throttle1->OnGetConnectedPeers(true, peers);
  throttle2->OnGetConnectedPeers(true, peers);
  throttle3->OnGetConnectedPeers(true, peers);
  EXPECT_TRUE(was_navigation_resumed1);
  EXPECT_TRUE(was_navigation_resumed2);
  EXPECT_TRUE(was_navigation_resumed3);
}

TEST_F(IpfsNavigationThrottleUnitTest, SequentialRequests) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod, static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));

  auto* service = ipfs_service(profile());
  ASSERT_TRUE(service);
  service->SetSkipGetConnectedPeersCallbackForTest(true);
  service->SetGetConnectedPeersCalledForTest(false);
  bool was_navigation_resumed1 = false;
  auto throttle1 = CreateDeferredNavigation(
      service,
      base::BindLambdaForTesting([&]() { was_navigation_resumed1 = true; }));

  bool was_navigation_resumed2 = false;
  auto throttle2 = CreateDeferredNavigation(
      service,
      base::BindLambdaForTesting([&]() { was_navigation_resumed2 = true; }));

  service->SetAllowIpfsLaunchForTest(true);
  service->GetIpnsKeysManager()->SetLoadCallbackForTest(base::DoNothing());
  service->RunLaunchDaemonCallbackForTest(true);
  throttle1->OnIpfsLaunched(true);
  EXPECT_FALSE(was_navigation_resumed1);
  EXPECT_FALSE(was_navigation_resumed2);

  auto peers = std::vector<std::string>();
  throttle1->OnGetConnectedPeers(true, peers);
  EXPECT_FALSE(was_navigation_resumed1);
  EXPECT_FALSE(was_navigation_resumed2);

  peers = std::vector<std::string>{
      "/ip4/101.101.101.101/tcp/4001/p2p/"
      "QmaCpDMGvV2BGHeYERUEnRQAwe3N8SzbUtfsmvsqQLuvuJ"};
  throttle1->OnGetConnectedPeers(true, peers);
  EXPECT_TRUE(was_navigation_resumed1);
  EXPECT_FALSE(was_navigation_resumed2);
  throttle2->OnGetConnectedPeers(true, peers);
  EXPECT_TRUE(was_navigation_resumed2);
  auto throttle3 = CreateDeferredNavigation(service, base::RepeatingClosure());

  ASSERT_TRUE(service->WasConnectedPeersCalledForTest());
}

TEST_F(IpfsNavigationThrottleUnitTest, DeferUntilPeersFetched) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod, static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));
  auto* service = ipfs_service(profile());
  ASSERT_TRUE(service);
  service->SetSkipGetConnectedPeersCallbackForTest(true);

  service->SetAllowIpfsLaunchForTest(true);
  service->GetIpnsKeysManager()->SetLoadCallbackForTest(base::DoNothing());
  service->RunLaunchDaemonCallbackForTest(true);

  bool was_navigation_resumed1 = false;
  auto throttle1 = CreateDeferredNavigation(
      service,
      base::BindLambdaForTesting([&]() { was_navigation_resumed1 = true; }));

  bool was_navigation_resumed2 = false;
  auto throttle2 = CreateDeferredNavigation(
      service,
      base::BindLambdaForTesting([&]() { was_navigation_resumed2 = true; }));
  EXPECT_FALSE(was_navigation_resumed1);
  EXPECT_FALSE(was_navigation_resumed2);

  auto peers = std::vector<std::string>();
  throttle1->OnGetConnectedPeers(true, peers);
  EXPECT_FALSE(was_navigation_resumed1);
  EXPECT_FALSE(was_navigation_resumed2);

  throttle2->OnGetConnectedPeers(true, peers);
  EXPECT_FALSE(was_navigation_resumed1);
  EXPECT_FALSE(was_navigation_resumed2);

  peers = std::vector<std::string>{
      "/ip4/101.101.101.101/tcp/4001/p2p/"
      "QmaCpDMGvV2BGHeYERUEnRQAwe3N8SzbUtfsmvsqQLuvuJ"};
  throttle1->OnGetConnectedPeers(true, peers);
  EXPECT_TRUE(was_navigation_resumed1);
  EXPECT_FALSE(was_navigation_resumed2);

  throttle2->OnGetConnectedPeers(true, peers);
  EXPECT_TRUE(was_navigation_resumed1);
  EXPECT_TRUE(was_navigation_resumed2);
}

TEST_F(IpfsNavigationThrottleUnitTest, DeferUntilIpfsProcessLaunched) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod, static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));

  auto peers = std::vector<std::string>{
      "/ip4/101.101.101.101/tcp/4001/p2p/"
      "QmaCpDMGvV2BGHeYERUEnRQAwe3N8SzbUtfsmvsqQLuvuJ"};

  auto* service = ipfs_service(profile());
  ASSERT_TRUE(service);
  service->SetSkipGetConnectedPeersCallbackForTest(true);
  service->GetIpnsKeysManager()->SetLoadCallbackForTest(base::DoNothing());

  content::MockNavigationHandle test_handle(web_contents());
  test_handle.set_url(GetIPFSURL());
  auto throttle = IpfsNavigationThrottle::MaybeCreateThrottleFor(
      &test_handle, service, profile()->GetPrefs(), locale());
  ASSERT_TRUE(throttle != nullptr);
  bool was_navigation_resumed = false;
  throttle->set_resume_callback_for_testing(
      base::BindLambdaForTesting([&]() { was_navigation_resumed = true; }));
  EXPECT_EQ(NavigationThrottle::DEFER, throttle->WillStartRequest().action())
      << GetIPFSURL();
  service->SetAllowIpfsLaunchForTest(true);
  service->RunLaunchDaemonCallbackForTest(true);
  EXPECT_FALSE(was_navigation_resumed);

  was_navigation_resumed = false;
  EXPECT_EQ(NavigationThrottle::DEFER, throttle->WillStartRequest().action())
      << GetIPFSURL();
  throttle->OnGetConnectedPeers(true, peers);
  EXPECT_TRUE(was_navigation_resumed);

  service->SetAllowIpfsLaunchForTest(false);

  was_navigation_resumed = false;
  test_handle.set_url(GetIPNSURL());
  EXPECT_EQ(NavigationThrottle::DEFER, throttle->WillStartRequest().action())
      << GetIPNSURL();
  service->SetAllowIpfsLaunchForTest(true);
  service->RunLaunchDaemonCallbackForTest(true);
  EXPECT_FALSE(was_navigation_resumed);

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
      &test_handle, ipfs_service(profile()), profile()->GetPrefs(), locale());
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
      &test_handle, ipfs_service(profile()), profile()->GetPrefs(), locale());
  ASSERT_TRUE(throttle != nullptr);
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << GetIPFSURL();

  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_DISABLED));
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << GetIPFSURL();
}

TEST_F(IpfsNavigationThrottleUnitTest, ProceedForNonLocalGatewayURL) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod, static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));

  content::MockNavigationHandle test_handle(web_contents());
  test_handle.set_url(GetPublicGatewayURL());
  auto throttle = IpfsNavigationThrottle::MaybeCreateThrottleFor(
      &test_handle, ipfs_service(profile()), profile()->GetPrefs(), locale());
  ASSERT_TRUE(throttle != nullptr);
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << GetPublicGatewayURL();

  test_handle.set_url(GetNonIPFSURL());
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << GetNonIPFSURL();
}

TEST_F(IpfsNavigationThrottleUnitTest, Instantiation) {
  content::MockNavigationHandle test_handle(web_contents());
  auto throttle = IpfsNavigationThrottle::MaybeCreateThrottleFor(
      &test_handle, ipfs_service(profile()), profile()->GetPrefs(), locale());
  EXPECT_TRUE(throttle != nullptr);

  // Disable in OTR profile.
  auto otr_web_contents = content::WebContentsTester::CreateTestWebContents(
      profile()->GetPrimaryOTRProfile(/*create_if_needed=*/true), nullptr);
  content::MockNavigationHandle otr_test_handle(otr_web_contents.get());
  auto throttle_in_otr = IpfsNavigationThrottle::MaybeCreateThrottleFor(
      &otr_test_handle,
      ipfs_service(profile()->GetPrimaryOTRProfile(/*create_if_needed=*/true)),
      profile()->GetPrefs(), locale());
  EXPECT_EQ(throttle_in_otr, nullptr);

  // Disable in guest sessions.
  auto guest_profile = CreateGuestProfile();
  auto guest_web_contents = content::WebContentsTester::CreateTestWebContents(
      guest_profile.get(), nullptr);
  content::MockNavigationHandle guest_test_handle(guest_web_contents.get());
  auto throttle_in_guest = IpfsNavigationThrottle::MaybeCreateThrottleFor(
      &guest_test_handle, ipfs_service(guest_profile.get()),
      profile()->GetPrefs(), locale());
  EXPECT_EQ(throttle_in_guest, nullptr);
}

#if BUILDFLAG(ENABLE_TOR)
TEST_F(IpfsNavigationThrottleUnitTest, NotInstantiatedInTor) {
  Profile* tor_profile =
      TorProfileManager::GetInstance().GetTorProfile(profile());
  ASSERT_TRUE(tor_profile->IsTor());
  ASSERT_TRUE(tor_profile->IsOffTheRecord());
  ASSERT_EQ(tor_profile->GetOriginalProfile(), profile());

  auto tor_web_contents =
      content::WebContentsTester::CreateTestWebContents(tor_profile, nullptr);
  content::MockNavigationHandle tor_test_handle(tor_web_contents.get());
  auto throttle_in_tor = IpfsNavigationThrottle::MaybeCreateThrottleFor(
      &tor_test_handle, ipfs_service(tor_profile), profile()->GetPrefs(),
      locale());
  EXPECT_EQ(throttle_in_tor, nullptr);
}
#endif

}  // namespace ipfs
