/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/tor_navigation_throttle.h"

#include "base/test/bind.h"
#include "brave/components/tor/mock_tor_launcher_factory.h"
#include "brave/components/tor/tor_launcher_factory.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/mock_navigation_throttle_registry.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using content::NavigationThrottle;

namespace tor {

class TorNavigationThrottleUnitTest : public testing::Test {
 public:
  TorNavigationThrottleUnitTest() = default;
  TorNavigationThrottleUnitTest(const TorNavigationThrottleUnitTest&) = delete;
  TorNavigationThrottleUnitTest& operator=(
      const TorNavigationThrottleUnitTest&) = delete;
  ~TorNavigationThrottleUnitTest() override = default;

  void SetUp() override {
    // The 2-arg MaybeCreateAndAdd used in production resolves the launcher
    // through TorLauncherFactory::GetInstance().
    TorLauncherFactory::SetTorLauncherFactoryForTesting(
        GetTorLauncherFactory());
  }

  void TearDown() override {
    TorLauncherFactory::SetTorLauncherFactoryForTesting(nullptr);
  }

  MockTorLauncherFactory* GetTorLauncherFactory() {
    return &MockTorLauncherFactory::GetInstance();
  }

  content::NavigationThrottle::ThrottleCheckResult GetThrottleResult(
      content::MockNavigationThrottleRegistry& registry) {
    const auto start = registry.throttles().back()->WillStartRequest().action();
    const auto redirect =
        registry.throttles().back()->WillRedirectRequest().action();
    EXPECT_EQ(start, redirect);
    return start;
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
};

// Tests TorNavigationThrottle::MaybeCreateThrottleFor with tor enabled/disabled
TEST_F(TorNavigationThrottleUnitTest, Instantiation) {
  content::MockNavigationHandle test_handle;
  content::MockNavigationThrottleRegistry registry(
      &test_handle,
      content::MockNavigationThrottleRegistry::RegistrationMode::kHold);
  TorNavigationThrottle::MaybeCreateAndAdd(registry, /*is_tor_profile=*/true);
  EXPECT_FALSE(registry.throttles().empty());

  content::MockNavigationHandle test_handle2;
  content::MockNavigationThrottleRegistry registry2(
      &test_handle2,
      content::MockNavigationThrottleRegistry::RegistrationMode::kHold);
  TorNavigationThrottle::MaybeCreateAndAdd(registry2,
                                           /*is_tor_profile=*/false);
  EXPECT_TRUE(registry2.throttles().empty());
}

TEST_F(TorNavigationThrottleUnitTest, WhitelistedScheme) {
  testing::Mock::AllowLeak(GetTorLauncherFactory());
  EXPECT_CALL(*GetTorLauncherFactory(), IsTorConnected)
      .WillRepeatedly(testing::Return(true));
  content::MockNavigationHandle test_handle;
  content::MockNavigationThrottleRegistry registry(
      &test_handle,
      content::MockNavigationThrottleRegistry::RegistrationMode::kHold);
  TorNavigationThrottle::MaybeCreateAndAdd(registry, *GetTorLauncherFactory(),
                                           /*is_tor_profile=*/true);
  ASSERT_FALSE(registry.throttles().empty());
  GURL url("http://www.example.com");
  test_handle.set_url(url);
  EXPECT_EQ(NavigationThrottle::PROCEED, GetThrottleResult(registry)) << url;

  GURL url2("https://www.example.com");
  test_handle.set_url(url2);
  EXPECT_EQ(NavigationThrottle::PROCEED, GetThrottleResult(registry)) << url2;

  GURL url3("chrome://settings");
  test_handle.set_url(url3);
  EXPECT_EQ(NavigationThrottle::PROCEED, GetThrottleResult(registry)) << url3;

  GURL url4("chrome-extension://cldoidikboihgcjfkhdeidbpclkineef");
  test_handle.set_url(url4);
  EXPECT_EQ(NavigationThrottle::PROCEED, GetThrottleResult(registry)) << url4;

  // chrome-devtools migrates to devtools
  GURL url5("devtools://devtools/bundled/inspector.html");
  test_handle.set_url(url5);
  EXPECT_EQ(NavigationThrottle::PROCEED, GetThrottleResult(registry)) << url5;
}

// Every schemes other than whitelisted scheme, no matter it is internal or
// external scheme
TEST_F(TorNavigationThrottleUnitTest, BlockedScheme) {
  testing::Mock::AllowLeak(GetTorLauncherFactory());
  EXPECT_CALL(*GetTorLauncherFactory(), IsTorConnected)
      .WillRepeatedly(testing::Return(true));
  content::MockNavigationHandle test_handle;
  content::MockNavigationThrottleRegistry registry(
      &test_handle,
      content::MockNavigationThrottleRegistry::RegistrationMode::kHold);
  TorNavigationThrottle::MaybeCreateAndAdd(registry, *GetTorLauncherFactory(),
                                           /*is_tor_profile=*/true);
  ASSERT_FALSE(registry.throttles().empty());
  GURL url("ftp://ftp.example.com");
  test_handle.set_url(url);
  EXPECT_EQ(NavigationThrottle::BLOCK_REQUEST, GetThrottleResult(registry))
      << url;

  GURL url2("mailto:example@www.example.com");
  test_handle.set_url(url2);
  EXPECT_EQ(NavigationThrottle::BLOCK_REQUEST, GetThrottleResult(registry))
      << url2;

  GURL url3("magnet:?xt=urn:btih:***.torrent");
  test_handle.set_url(url3);
  EXPECT_EQ(NavigationThrottle::BLOCK_REQUEST, GetThrottleResult(registry))
      << url3;
}

TEST_F(TorNavigationThrottleUnitTest, DeferUntilTorProcessLaunched) {
  testing::Mock::AllowLeak(GetTorLauncherFactory());
  EXPECT_CALL(*GetTorLauncherFactory(), IsTorConnected)
      .WillRepeatedly(testing::Return(false));
  content::MockNavigationHandle test_handle;
  content::MockNavigationThrottleRegistry registry(
      &test_handle,
      content::MockNavigationThrottleRegistry::RegistrationMode::kHold);
  TorNavigationThrottle::MaybeCreateAndAdd(registry, *GetTorLauncherFactory(),
                                           /*is_tor_profile=*/true);
  ASSERT_FALSE(registry.throttles().empty());
  bool was_navigation_resumed = false;
  registry.throttles().back()->set_resume_callback_for_testing(
      base::BindLambdaForTesting([&]() { was_navigation_resumed = true; }));
  GURL url("http://www.example.com");
  test_handle.set_url(url);
  EXPECT_EQ(NavigationThrottle::DEFER, GetThrottleResult(registry)) << url;
  GURL url2("chrome://newtab");
  test_handle.set_url(url2);
  EXPECT_EQ(NavigationThrottle::PROCEED, GetThrottleResult(registry)) << url2;
  static_cast<TorNavigationThrottle*>(registry.throttles().back().get())
      ->OnTorCircuitEstablished(true);
  EXPECT_TRUE(was_navigation_resumed);
  EXPECT_CALL(*GetTorLauncherFactory(), IsTorConnected)
      .WillRepeatedly(testing::Return(true));
  EXPECT_EQ(NavigationThrottle::PROCEED, GetThrottleResult(registry)) << url;
}

}  // namespace tor
