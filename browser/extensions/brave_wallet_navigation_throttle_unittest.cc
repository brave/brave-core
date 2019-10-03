/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_wallet_navigation_throttle.h"

#include <memory>
#include <utility>
#include <vector>

#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/profiles/tor_unittest_profile_manager.h"
#include "brave/browser/tor/buildflags.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/common/content_client.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/web_contents_tester.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/common/tor/pref_names.h"
#endif

using content::NavigationThrottle;

namespace extensions {

namespace {

class MockBrowserClient : public content::ContentBrowserClient {
 public:
  MockBrowserClient() {}
  ~MockBrowserClient() override {}

  // Only construct an BraveWalletNavigationThrottle so that we can test it in
  // isolation.
  std::vector<std::unique_ptr<NavigationThrottle>> CreateThrottlesForNavigation(
      content::NavigationHandle* handle) override {
    std::vector<std::unique_ptr<NavigationThrottle>> throttles;
    throttles.push_back(
        std::make_unique<BraveWalletNavigationThrottle>(handle));
    return throttles;
  }
};

}  // namespace

class BraveWalletNavigationThrottleUnitTest
    : public ChromeRenderViewHostTestHarness {
 public:
  BraveWalletNavigationThrottleUnitTest()
      : local_state_(TestingBrowserProcess::GetGlobal()) {
  }

  void SetUp() override {
#if BUILDFLAG(ENABLE_TOR)
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    ProfileManager* profile_manager
      = new TorUnittestProfileManager(temp_dir_.GetPath());
    TestingBrowserProcess::GetGlobal()->SetProfileManager(profile_manager);
    profile_manager->InitProfileUserPrefs(
        ProfileManager::GetActiveUserProfile());
    ASSERT_TRUE(profile_manager);
#endif
    original_client_ = content::SetBrowserClientForTesting(&client_);
    ChromeRenderViewHostTestHarness::SetUp();
  }

  content::BrowserContext* CreateBrowserContext() override {
    TestingProfile::Builder builder;
    auto prefs =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    RegisterUserProfilePrefs(prefs->registry());
    builder.SetPrefService(std::move(prefs));
    return builder.Build().release();
  }

  void TearDown() override {
#if BUILDFLAG(ENABLE_TOR)
    TestingBrowserProcess::GetGlobal()->SetProfileManager(nullptr);
#endif
    content::SetBrowserClientForTesting(original_client_);
    ChromeRenderViewHostTestHarness::TearDown();
  }

  content::RenderFrameHostTester* render_frame_host_tester(
      content::RenderFrameHost* host) {
    return content::RenderFrameHostTester::For(host);
  }

  content::WebContentsTester* web_contents_tester() {
    return content::WebContentsTester::For(web_contents());
  }

  void AddExtension() {
    DictionaryBuilder manifest;
    manifest.Set("name", "ext")
        .Set("version", "0.1")
        .Set("manifest_version", 2);
    extension_ = ExtensionBuilder()
                     .SetManifest(manifest.Build())
                     .SetID(ethereum_remote_client_extension_id)
                     .Build();
    ASSERT_TRUE(extension_);
    ExtensionRegistry::Get(browser_context())->AddReady(extension_.get());
  }

 private:
  scoped_refptr<const Extension> extension_;
  MockBrowserClient client_;
  content::ContentBrowserClient* original_client_;
  ScopedTestingLocalState local_state_;
  base::ScopedTempDir temp_dir_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  DISALLOW_COPY_AND_ASSIGN(BraveWalletNavigationThrottleUnitTest);
};

// Tests the basic case of loading a URL, it should proceed.
TEST_F(BraveWalletNavigationThrottleUnitTest, ExternalWebPage) {
  web_contents_tester()->NavigateAndCommit(GURL("http://example.com"));
  content::RenderFrameHost* host =
      render_frame_host_tester(main_rfh())->AppendChild("child");
  GURL url("http://www.example.com");
  content::MockNavigationHandle test_handle(url, host);
  test_handle.set_starting_site_instance(host->GetSiteInstance());
  auto throttle = std::make_unique<BraveWalletNavigationThrottle>(&test_handle);
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << url;
}

// Tests the case of loading brave://settings without the extension installed.
// It should just proceed.
TEST_F(BraveWalletNavigationThrottleUnitTest, DifferentChromePageWithExt) {
  web_contents_tester()->NavigateAndCommit(GURL("http://example.com"));
  content::RenderFrameHost* host =
      render_frame_host_tester(main_rfh())->AppendChild("child");
  GURL url("chrome://settings");
  content::MockNavigationHandle test_handle(url, host);
  test_handle.set_starting_site_instance(host->GetSiteInstance());
  auto throttle = std::make_unique<BraveWalletNavigationThrottle>(&test_handle);
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << url;
}

// Tests the case of loading brave://wallet without having the extension
// installed.a It should defer which it does to install the extension.
TEST_F(BraveWalletNavigationThrottleUnitTest, ChromeWalletUrlNotInstalled) {
  web_contents_tester()->NavigateAndCommit(GURL("http://example.com"));
  content::RenderFrameHost* host =
      render_frame_host_tester(main_rfh())->AppendChild("child");
  GURL url("chrome://wallet");
  content::MockNavigationHandle test_handle(url, host);
  test_handle.set_starting_site_instance(host->GetSiteInstance());
  auto throttle = std::make_unique<BraveWalletNavigationThrottle>(&test_handle);
  EXPECT_EQ(NavigationThrottle::DEFER, throttle->WillStartRequest().action())
      << url;
}

// Tests the case of loading brave://wallet with the extension installed.
// It should just proceed.
TEST_F(BraveWalletNavigationThrottleUnitTest, ChromeWalletUrlInstalled) {
  AddExtension();
  web_contents_tester()->NavigateAndCommit(GURL("http://example.com"));
  content::RenderFrameHost* host =
      render_frame_host_tester(main_rfh())->AppendChild("child");
  GURL url("chrome://wallet");
  content::MockNavigationHandle test_handle(url, host);
  test_handle.set_starting_site_instance(host->GetSiteInstance());
  auto throttle = std::make_unique<BraveWalletNavigationThrottle>(&test_handle);
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << url;
}

// Tests the case of loading brave://wallet when the Wallet is explicitly
// disabled.
TEST_F(BraveWalletNavigationThrottleUnitTest, ChromeWalletDisabledByPref) {
  profile()->GetPrefs()->SetBoolean(kBraveWalletEnabled, false);
  web_contents_tester()->NavigateAndCommit(GURL("http://example.com"));
  content::RenderFrameHost* host =
      render_frame_host_tester(main_rfh())->AppendChild("child");
  GURL url("chrome://wallet");
  content::MockNavigationHandle test_handle(url, host);
  test_handle.set_starting_site_instance(host->GetSiteInstance());
  auto throttle = std::make_unique<BraveWalletNavigationThrottle>(&test_handle);
  EXPECT_EQ(NavigationThrottle::BLOCK_REQUEST,
      throttle->WillStartRequest().action()) << url;
}

#if BUILDFLAG(ENABLE_TOR)
// Make sure Brave Wallet is not available in a Tor profile.
TEST_F(BraveWalletNavigationThrottleUnitTest,
    ChromeWalletNotAvailInTorProfile) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  Profile* profile = ProfileManager::GetLastUsedProfile();
  Profile* tor_profile = profile_manager->GetProfile(
      BraveProfileManager::GetTorProfilePath());
  ASSERT_EQ(brave::GetParentProfile(tor_profile), profile);
  std::unique_ptr<content::WebContents> tor_web_contents =
      content::WebContentsTester::CreateTestWebContents(tor_profile, nullptr);

  content::WebContentsTester::For(tor_web_contents.get())->
      NavigateAndCommit(GURL("http://example.com"));
  content::RenderFrameHost* host =
      render_frame_host_tester(tor_web_contents->GetMainFrame())->
          AppendChild("child");
  GURL url("chrome://wallet");
  content::MockNavigationHandle test_handle(url, host);
  test_handle.set_starting_site_instance(host->GetSiteInstance());
  auto throttle = std::make_unique<BraveWalletNavigationThrottle>(&test_handle);
  EXPECT_EQ(NavigationThrottle::BLOCK_REQUEST,
      throttle->WillStartRequest().action()) << url;
  tor_web_contents.reset();
}
#endif

}  // namespace extensions
