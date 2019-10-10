/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_navigation_throttle.h"

#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/browser/profiles/tor_unittest_profile_manager.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using content::NavigationThrottle;

namespace tor {

class TorNavigationThrottleUnitTest : public testing::Test {
 public:
  TorNavigationThrottleUnitTest()
      : local_state_(TestingBrowserProcess::GetGlobal()) {}
  ~TorNavigationThrottleUnitTest() override = default;

  void SetUp() override {
    // Create a new temporary directory, and store the path
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    TestingBrowserProcess::GetGlobal()->SetProfileManager(
        new TorUnittestProfileManager(temp_dir_.GetPath()));
    // Create profile.
    ProfileManager* profile_manager = g_browser_process->profile_manager();
    ASSERT_TRUE(profile_manager);
    Profile* profile = profile_manager->GetProfile(
        temp_dir_.GetPath().AppendASCII(TestingProfile::kTestUserProfileDir));
    Profile* tor_profile = profile_manager->GetProfile(
        BraveProfileManager::GetTorProfilePath());
    web_contents_ =
        content::WebContentsTester::CreateTestWebContents(profile, nullptr);
    tor_web_contents_ =
        content::WebContentsTester::CreateTestWebContents(tor_profile, nullptr);
  }

  void TearDown() override {
    tor_web_contents_.reset();
    web_contents_.reset();
    TestingBrowserProcess::GetGlobal()->SetProfileManager(nullptr);
  }

  content::WebContents* web_contents() {
    return web_contents_.get();
  }

  content::WebContents* tor_web_contents() {
    return tor_web_contents_.get();
  }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  // The path to temporary directory used to contain the test operations.
  base::ScopedTempDir temp_dir_;
  ScopedTestingLocalState local_state_;
  std::unique_ptr<content::WebContents> web_contents_;
  std::unique_ptr<content::WebContents> tor_web_contents_;
  DISALLOW_COPY_AND_ASSIGN(TorNavigationThrottleUnitTest);
};

// Tests TorNavigationThrottle::MaybeCreateThrottleFor with tor enabled/disabled
TEST_F(TorNavigationThrottleUnitTest, Instantiation) {
  content::MockNavigationHandle test_handle(tor_web_contents());
  std::unique_ptr<TorNavigationThrottle> throttle =
    TorNavigationThrottle::MaybeCreateThrottleFor(&test_handle);
  EXPECT_TRUE(throttle != nullptr);

  content::MockNavigationHandle test_handle2(web_contents());
  std::unique_ptr<TorNavigationThrottle> throttle2 =
    TorNavigationThrottle::MaybeCreateThrottleFor(&test_handle2);
  EXPECT_TRUE(throttle2 == nullptr);
}

TEST_F(TorNavigationThrottleUnitTest, WhitelistedScheme) {
  content::MockNavigationHandle test_handle(tor_web_contents());
  std::unique_ptr<TorNavigationThrottle> throttle =
    TorNavigationThrottle::MaybeCreateThrottleFor(&test_handle);
  GURL url("http://www.example.com");
  test_handle.set_url(url);
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
  content::MockNavigationHandle test_handle(tor_web_contents());
  std::unique_ptr<TorNavigationThrottle> throttle =
    TorNavigationThrottle::MaybeCreateThrottleFor(&test_handle);
  GURL url("ftp://ftp.example.com");
  test_handle.set_url(url);
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
