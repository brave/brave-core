/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_webtorrent_navigation_throttle.h"

#include <memory>
#include <utility>
#include <vector>

#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/browser/profiles/profile_util.h"
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

using content::NavigationThrottle;

namespace extensions {

namespace {

class MockBrowserClient : public content::ContentBrowserClient {
 public:
  MockBrowserClient() {}
  ~MockBrowserClient() override {}

  // Only construct an BraveWebTorrentNavigationThrottle so that we can test it
  // in isolation.
  std::vector<std::unique_ptr<NavigationThrottle>> CreateThrottlesForNavigation(
      content::NavigationHandle* handle) override {
    std::vector<std::unique_ptr<NavigationThrottle>> throttles;
    throttles.push_back(
        std::make_unique<BraveWebTorrentNavigationThrottle>(handle));
    return throttles;
  }
};

GURL magnet_url("magnet:?xt=urn:btih:dd8255ecdc7ca55fb0bbf81323d87062db1f6d1c&dn=Big+Buck+Bunny&tr=udp%3A%2F%2Fexplodie.org%3A6969&tr=udp%3A%2F%2Ftracker.coppersurfer.tk%3A6969&tr=udp%3A%2F%2Ftracker.empire-js.us%3A1337&tr=udp%3A%2F%2Ftracker.leechers-paradise.org%3A6969&tr=udp%3A%2F%2Ftracker.opentrackr.org%3A1337&tr=wss%3A%2F%2Ftracker.btorrent.xyz&tr=wss%3A%2F%2Ftracker.fastcast.nz&tr=wss%3A%2F%2Ftracker.openwebtorrent.com&ws=https%3A%2F%2Fwebtorrent.io%2Ftorrents%2F&xs=https%3A%2F%2Fwebtorrent.io%2Ftorrents%2Fbig-buck-bunny.torrent");  // NOLINT
GURL torrent_url("https://webtorrent.io/torrents/big-buck-bunny.torrent");

}  // namespace

class BraveWebTorrentNavigationThrottleUnitTest
    : public ChromeRenderViewHostTestHarness {
 public:
  BraveWebTorrentNavigationThrottleUnitTest()
      : local_state_(TestingBrowserProcess::GetGlobal()) {
  }

  void SetUp() override {
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
                     .SetID(brave_webtorrent_extension_id)
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
  DISALLOW_COPY_AND_ASSIGN(BraveWebTorrentNavigationThrottleUnitTest);
};

// Tests the basic case of loading a URL, it should proceed.
TEST_F(BraveWebTorrentNavigationThrottleUnitTest, ExternalWebPage) {
  web_contents_tester()->NavigateAndCommit(GURL("http://example.com"));
  content::RenderFrameHost* host =
      render_frame_host_tester(main_rfh())->AppendChild("child");
  GURL url("http://www.example.com");
  content::MockNavigationHandle test_handle(url, host);
  test_handle.set_starting_site_instance(host->GetSiteInstance());
  auto throttle =
      std::make_unique<BraveWebTorrentNavigationThrottle>(&test_handle);
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << url;
}

// Tests the case of loading a torrent without having the extension
// installed. It should defer which it does to install the extension.
TEST_F(BraveWebTorrentNavigationThrottleUnitTest,
    WebTorrentUrlNotInstalled) {
  web_contents_tester()->NavigateAndCommit(GURL("http://example.com"));
  content::RenderFrameHost* host =
      render_frame_host_tester(main_rfh())->AppendChild("child");
  content::MockNavigationHandle test_handle(torrent_url, host);
  test_handle.set_starting_site_instance(host->GetSiteInstance());
  auto throttle =
      std::make_unique<BraveWebTorrentNavigationThrottle>(&test_handle);
  EXPECT_EQ(NavigationThrottle::DEFER, throttle->WillStartRequest().action())
      << torrent_url;
}

// Tests the case of loading a torrent without having the extension
// installed. It should defer which it does to install the extension.
TEST_F(BraveWebTorrentNavigationThrottleUnitTest,
    WebTorrentMagnetUrlNotInstalled) {
  web_contents_tester()->NavigateAndCommit(GURL("http://example.com"));
  content::RenderFrameHost* host =
      render_frame_host_tester(main_rfh())->AppendChild("child");
  content::MockNavigationHandle test_handle(magnet_url, host);
  test_handle.set_starting_site_instance(host->GetSiteInstance());
  auto throttle =
      std::make_unique<BraveWebTorrentNavigationThrottle>(&test_handle);
  EXPECT_EQ(NavigationThrottle::DEFER, throttle->WillStartRequest().action())
      << magnet_url;
}


// Tests the case of loading a torrent with the extension installed.
// It should just proceed.
TEST_F(BraveWebTorrentNavigationThrottleUnitTest, WebTorrentUrlInstalled) {
  AddExtension();
  web_contents_tester()->NavigateAndCommit(GURL("http://example.com"));
  content::RenderFrameHost* host =
      render_frame_host_tester(main_rfh())->AppendChild("child");
  content::MockNavigationHandle test_handle(magnet_url, host);
  test_handle.set_starting_site_instance(host->GetSiteInstance());
  auto throttle =
      std::make_unique<BraveWebTorrentNavigationThrottle>(&test_handle);
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << magnet_url;
}

// Tests the case of loading a torrent when the WebTorrent is explicitly
// disabled.
TEST_F(BraveWebTorrentNavigationThrottleUnitTest, WebTorrentDisabledByPref) {
  profile()->GetPrefs()->SetBoolean(kWebTorrentEnabled, false);
  web_contents_tester()->NavigateAndCommit(GURL("http://example.com"));
  content::RenderFrameHost* host =
      render_frame_host_tester(main_rfh())->AppendChild("child");
  content::MockNavigationHandle test_handle(magnet_url, host);
  test_handle.set_starting_site_instance(host->GetSiteInstance());
  auto throttle =
      std::make_unique<BraveWebTorrentNavigationThrottle>(&test_handle);
  EXPECT_EQ(NavigationThrottle::PROCEED,
      throttle->WillStartRequest().action()) << magnet_url;
}

}  // namespace extensions
