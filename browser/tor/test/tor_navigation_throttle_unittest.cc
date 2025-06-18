/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/tor_navigation_throttle.h"

#include <memory>
#include <utility>

#include "base/test/bind.h"
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/components/tor/mock_tor_launcher_factory.h"
#include "brave/components/tor/tor_profile_service.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/mock_navigation_throttle_registry.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using content::NavigationThrottle;

namespace tor {
namespace {
constexpr char kTestProfileName[] = "TestProfile";
}  // namespace

class TorNavigationThrottleUnitTest : public testing::Test {
 public:
  TorNavigationThrottleUnitTest() = default;
  TorNavigationThrottleUnitTest(const TorNavigationThrottleUnitTest&) = delete;
  TorNavigationThrottleUnitTest& operator=(
      const TorNavigationThrottleUnitTest&) = delete;
  ~TorNavigationThrottleUnitTest() override = default;

  void SetUp() override {
    TestingBrowserProcess* browser_process = TestingBrowserProcess::GetGlobal();
    profile_manager_ = std::make_unique<TestingProfileManager>(browser_process);
    ASSERT_TRUE(profile_manager_->SetUp());
    Profile* profile = profile_manager_->CreateTestingProfile(kTestProfileName);
    Profile* tor_profile =
        TorProfileManager::GetInstance().GetTorProfile(profile);
    ASSERT_EQ(tor_profile->GetOriginalProfile(), profile);
    web_contents_ =
        content::WebContentsTester::CreateTestWebContents(profile, nullptr);
    tor_web_contents_ =
        content::WebContentsTester::CreateTestWebContents(tor_profile, nullptr);
    TorProfileService* tor_profile_service =
        TorProfileServiceFactory::GetForContext(tor_profile);
    tor_profile_service->SetTorLauncherFactoryForTest(GetTorLauncherFactory());
    ASSERT_EQ(TorProfileServiceFactory::GetForContext(profile), nullptr);
  }

  void TearDown() override {
    tor_web_contents_.reset();
    web_contents_.reset();
    profile_manager_->DeleteTestingProfile(kTestProfileName);
  }

  content::WebContents* web_contents() { return web_contents_.get(); }

  content::WebContents* tor_web_contents() { return tor_web_contents_.get(); }

  MockTorLauncherFactory* GetTorLauncherFactory() {
    return &MockTorLauncherFactory::GetInstance();
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  content::RenderViewHostTestEnabler test_render_host_factories_;
  std::unique_ptr<content::WebContents> web_contents_;
  std::unique_ptr<content::WebContents> tor_web_contents_;
  std::unique_ptr<TestingProfileManager> profile_manager_;
};

// Tests TorNavigationThrottle::MaybeCreateThrottleFor with tor enabled/disabled
TEST_F(TorNavigationThrottleUnitTest, Instantiation) {
  content::MockNavigationHandle test_handle(tor_web_contents());
  content::MockNavigationThrottleRegistry registry(
      &test_handle,
      content::MockNavigationThrottleRegistry::RegistrationMode::kHold);
  TorNavigationThrottle::MaybeCreateAndAdd(
      registry, tor_web_contents()->GetBrowserContext()->IsTor());
  EXPECT_FALSE(registry.throttles().empty());

  content::MockNavigationHandle test_handle2(web_contents());
  content::MockNavigationThrottleRegistry registry2(
      &test_handle2,
      content::MockNavigationThrottleRegistry::RegistrationMode::kHold);
  TorNavigationThrottle::MaybeCreateAndAdd(
      registry2, web_contents()->GetBrowserContext()->IsTor());
  EXPECT_TRUE(registry2.throttles().empty());
}

TEST_F(TorNavigationThrottleUnitTest, WhitelistedScheme) {
  testing::Mock::AllowLeak(GetTorLauncherFactory());
  EXPECT_CALL(*GetTorLauncherFactory(), IsTorConnected)
      .WillRepeatedly(testing::Return(true));
  content::MockNavigationHandle test_handle(tor_web_contents());
  content::MockNavigationThrottleRegistry registry(
      &test_handle,
      content::MockNavigationThrottleRegistry::RegistrationMode::kHold);
  TorNavigationThrottle::MaybeCreateAndAdd(
      registry, *GetTorLauncherFactory(),
      tor_web_contents()->GetBrowserContext()->IsTor());
  ASSERT_FALSE(registry.throttles().empty());
  GURL url("http://www.example.com");
  test_handle.set_url(url);
  EXPECT_EQ(NavigationThrottle::PROCEED,
            registry.throttles().back()->WillStartRequest().action())
      << url;

  GURL url2("https://www.example.com");
  test_handle.set_url(url2);
  EXPECT_EQ(NavigationThrottle::PROCEED,
            registry.throttles().back()->WillStartRequest().action())
      << url2;

  GURL url3("chrome://settings");
  test_handle.set_url(url3);
  EXPECT_EQ(NavigationThrottle::PROCEED,
            registry.throttles().back()->WillStartRequest().action())
      << url3;

  GURL url4("chrome-extension://cldoidikboihgcjfkhdeidbpclkineef");
  test_handle.set_url(url4);
  EXPECT_EQ(NavigationThrottle::PROCEED,
            registry.throttles().back()->WillStartRequest().action())
      << url4;

  // chrome-devtools migrates to devtools
  GURL url5("devtools://devtools/bundled/inspector.html");
  test_handle.set_url(url5);
  EXPECT_EQ(NavigationThrottle::PROCEED,
            registry.throttles().back()->WillStartRequest().action())
      << url5;
}

// Every schemes other than whitelisted scheme, no matter it is internal or
// external scheme
TEST_F(TorNavigationThrottleUnitTest, BlockedScheme) {
  testing::Mock::AllowLeak(GetTorLauncherFactory());
  EXPECT_CALL(*GetTorLauncherFactory(), IsTorConnected)
      .WillRepeatedly(testing::Return(true));
  content::MockNavigationHandle test_handle(tor_web_contents());
  content::MockNavigationThrottleRegistry registry(
      &test_handle,
      content::MockNavigationThrottleRegistry::RegistrationMode::kHold);
  TorNavigationThrottle::MaybeCreateAndAdd(
      registry, *GetTorLauncherFactory(),
      tor_web_contents()->GetBrowserContext()->IsTor());
  ASSERT_FALSE(registry.throttles().empty());
  GURL url("ftp://ftp.example.com");
  test_handle.set_url(url);
  EXPECT_EQ(NavigationThrottle::BLOCK_REQUEST,
            registry.throttles().back()->WillStartRequest().action())
      << url;

  GURL url2("mailto:example@www.example.com");
  test_handle.set_url(url2);
  EXPECT_EQ(NavigationThrottle::BLOCK_REQUEST,
            registry.throttles().back()->WillStartRequest().action())
      << url2;

  GURL url3("magnet:?xt=urn:btih:***.torrent");
  test_handle.set_url(url3);
  EXPECT_EQ(NavigationThrottle::BLOCK_REQUEST,
            registry.throttles().back()->WillStartRequest().action())
      << url3;
}

TEST_F(TorNavigationThrottleUnitTest, DeferUntilTorProcessLaunched) {
  testing::Mock::AllowLeak(GetTorLauncherFactory());
  EXPECT_CALL(*GetTorLauncherFactory(), IsTorConnected)
      .WillRepeatedly(testing::Return(false));
  content::MockNavigationHandle test_handle(tor_web_contents());
  content::MockNavigationThrottleRegistry registry(
      &test_handle,
      content::MockNavigationThrottleRegistry::RegistrationMode::kHold);
  TorNavigationThrottle::MaybeCreateAndAdd(
      registry, *GetTorLauncherFactory(),
      tor_web_contents()->GetBrowserContext()->IsTor());
  ASSERT_FALSE(registry.throttles().empty());
  bool was_navigation_resumed = false;
  registry.throttles().back()->set_resume_callback_for_testing(
      base::BindLambdaForTesting([&]() { was_navigation_resumed = true; }));
  GURL url("http://www.example.com");
  test_handle.set_url(url);
  EXPECT_EQ(NavigationThrottle::DEFER,
            registry.throttles().back()->WillStartRequest().action())
      << url;
  GURL url2("chrome://newtab");
  test_handle.set_url(url2);
  EXPECT_EQ(NavigationThrottle::PROCEED,
            registry.throttles().back()->WillStartRequest().action())
      << url2;
  static_cast<TorNavigationThrottle*>(registry.throttles().back().get())
      ->OnTorCircuitEstablished(true);
  EXPECT_TRUE(was_navigation_resumed);
  EXPECT_CALL(*GetTorLauncherFactory(), IsTorConnected)
      .WillRepeatedly(testing::Return(true));
  EXPECT_EQ(NavigationThrottle::PROCEED,
            registry.throttles().back()->WillStartRequest().action())
      << url;
}

}  // namespace tor
