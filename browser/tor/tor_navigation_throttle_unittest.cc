/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_navigation_throttle.h"

#include <utility>

#include "brave/common/tor/pref_names.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using content::NavigationThrottle;

namespace tor {

class TorNavigationThrottleUnitTest
    : public ChromeRenderViewHostTestHarness {
 public:
  TorNavigationThrottleUnitTest() = default;
  ~TorNavigationThrottleUnitTest() override = default;

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
  }

  void TearDown() override {
    ChromeRenderViewHostTestHarness::TearDown();
  }

  content::BrowserContext* CreateBrowserContext() override {
    TestingProfile::Builder builder;
    auto prefs =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    RegisterUserProfilePrefs(prefs->registry());
    prefs->registry()->
      RegisterBooleanPref(prefs::kProfileUsingTor, false);
    builder.SetPrefService(std::move(prefs));
    return builder.Build().release();
  }

  content::RenderFrameHostTester* render_frame_host_tester(
      content::RenderFrameHost* host) {
    return content::RenderFrameHostTester::For(host);
  }

  content::WebContentsTester* web_contents_tester() {
    return content::WebContentsTester::For(web_contents());
  }
 private:
  DISALLOW_COPY_AND_ASSIGN(TorNavigationThrottleUnitTest);
};

// Tests TorNavigationThrottle::MaybeCreateThrottleFor with tor enabled/disabled
TEST_F(TorNavigationThrottleUnitTest, Instantiation) {
  profile()->GetPrefs()->SetBoolean(prefs::kProfileUsingTor, true);
  web_contents_tester()->NavigateAndCommit(GURL("http://example.com"));
  content::RenderFrameHost* rfh = main_rfh();
  GURL url("http://www.example.com");
  content::MockNavigationHandle test_handle(url, rfh);
  std::unique_ptr<TorNavigationThrottle> throttle =
    TorNavigationThrottle::MaybeCreateThrottleFor(&test_handle);
  EXPECT_TRUE(throttle != nullptr);
  profile()->GetPrefs()->SetBoolean(prefs::kProfileUsingTor, false);
  std::unique_ptr<TorNavigationThrottle> throttle2 =
    TorNavigationThrottle::MaybeCreateThrottleFor(&test_handle);
  EXPECT_TRUE(throttle2 == nullptr);
}

TEST_F(TorNavigationThrottleUnitTest, WhitelistedScheme) {
  web_contents_tester()->NavigateAndCommit(GURL("http://example.com"));
  content::RenderFrameHost* rfh = main_rfh();
  GURL url("http://www.example.com");
  content::MockNavigationHandle test_handle(url, rfh);
  std::unique_ptr<TorNavigationThrottle> throttle =
    std::make_unique<TorNavigationThrottle>(&test_handle);
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << url;

  GURL url2("https://www.example.com");
  test_handle.set_url(url2);
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << url2;

  GURL url3("chrome://settings");
  test_handle.set_url(url3);
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << url3;

  GURL url4("chrome-extension://cldoidikboihgcjfkhdeidbpclkineef");
  test_handle.set_url(url4);
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << url4;

  // chrome-devtools migrates to devtools
  GURL url5("devtools://devtools/bundled/inspector.html");
  test_handle.set_url(url5);
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << url5;
}

// Every schemes other than whitelisted scheme, no matter it is internal or
// external scheme
TEST_F(TorNavigationThrottleUnitTest, BlockedScheme) {
  web_contents_tester()->NavigateAndCommit(GURL("http://example.com"));
  content::RenderFrameHost* rfh = main_rfh();
  GURL url("ftp://ftp.example.com");
  content::MockNavigationHandle test_handle(url, rfh);
  std::unique_ptr<TorNavigationThrottle> throttle =
    std::make_unique<TorNavigationThrottle>(&test_handle);
  EXPECT_EQ(NavigationThrottle::BLOCK_REQUEST,
            throttle->WillStartRequest().action()) << url;

  GURL url2("mailto:example@www.example.com");
  test_handle.set_url(url2);
  EXPECT_EQ(NavigationThrottle::BLOCK_REQUEST,
            throttle->WillStartRequest().action()) << url2;

  GURL url3("magnet:?xt=urn:btih:***.torrent");
  test_handle.set_url(url3);
  EXPECT_EQ(NavigationThrottle::BLOCK_REQUEST,
            throttle->WillStartRequest().action()) << url3;
}

}  // namespace tor
