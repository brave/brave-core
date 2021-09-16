/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "brave/components/speedreader/speedreader_rewriter_service.h"
#include "brave/components/speedreader/speedreader_throttle.h"
#include "brave/components/speedreader/speedreader_util.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/site_instance.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace speedreader {

namespace {
constexpr char kTestProfileName[] = "TestProfile";
}  // anonymous namespace

class SpeedreaderThrottleTest : public testing::Test {
 public:
  SpeedreaderThrottleTest() = default;
  ~SpeedreaderThrottleTest() override = default;
  SpeedreaderThrottleTest(const SpeedreaderThrottleTest&) = delete;
  SpeedreaderThrottleTest& operator=(const SpeedreaderThrottleTest&) = delete;

  void SetUp() override {
    profile_manager_ = std::make_unique<TestingProfileManager>(
        TestingBrowserProcess::GetGlobal());
    EXPECT_TRUE(profile_manager_->SetUp());
    profile_ = profile_manager_->CreateTestingProfile(kTestProfileName);
    auto instance = content::SiteInstance::CreateForURL(profile_, url());
    web_contents_ =
        content::WebContentsTester::CreateTestWebContents(profile_, instance);
  }

  void TearDown() override {
    web_contents_.reset();
    profile_manager_->DeleteTestingProfile(kTestProfileName);
  }

  GURL url() { return GURL("https://brave.com"); }

  TestingProfile* profile() { return profile_; }

  content::WebContents* web_contents() { return web_contents_.get(); }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(profile());
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<content::WebContents> web_contents_;
  std::unique_ptr<TestingProfileManager> profile_manager_;
  TestingProfile* profile_;
};

TEST_F(SpeedreaderThrottleTest, AllowThrottle) {
  auto runner = content::GetUIThreadTaskRunner({});
  std::unique_ptr<SpeedReaderThrottle> throttle =
      SpeedReaderThrottle::MaybeCreateThrottleFor(nullptr, content_settings(),
                                                  url(), false, runner);
  EXPECT_NE(throttle.get(), nullptr);
}

TEST_F(SpeedreaderThrottleTest, ToggleThrottle) {
  auto runner = content::GetUIThreadTaskRunner({});
  std::unique_ptr<SpeedReaderThrottle> throttle;

  speedreader::SetEnabledForSite(content_settings(), url(), false);
  throttle = SpeedReaderThrottle::MaybeCreateThrottleFor(
      nullptr, content_settings(), url(), true, runner);
  EXPECT_EQ(throttle.get(), nullptr);
  // no other domains are affected by the rule.
  throttle = SpeedReaderThrottle::MaybeCreateThrottleFor(
      nullptr, content_settings(), GURL("kevin.com"), true, runner);
  EXPECT_NE(throttle.get(), nullptr);

  speedreader::SetEnabledForSite(content_settings(), url(), true);
  throttle = SpeedReaderThrottle::MaybeCreateThrottleFor(
      nullptr, content_settings(), url(), true, runner);
  EXPECT_NE(throttle.get(), nullptr);
}

TEST_F(SpeedreaderThrottleTest, ThrottleIgnoreDisabled) {
  auto runner = content::GetUIThreadTaskRunner({});
  std::unique_ptr<SpeedReaderThrottle> throttle;

  speedreader::SetEnabledForSite(content_settings(), url(), false);

  throttle = SpeedReaderThrottle::MaybeCreateThrottleFor(
      nullptr, content_settings(), url(), true /* check_disabled_sites */,
      runner);
  EXPECT_EQ(throttle.get(), nullptr);

  throttle = SpeedReaderThrottle::MaybeCreateThrottleFor(
      nullptr, content_settings(), url(), false /* check_disabled_sites */,
      runner);
  EXPECT_NE(throttle.get(), nullptr);
}

TEST_F(SpeedreaderThrottleTest, ThrottleNestedURL) {
  auto runner = content::GetUIThreadTaskRunner({});
  std::unique_ptr<SpeedReaderThrottle> throttle;

  // Even though we call this function on SetSiteSpeedreadable, it should apply
  // to all of brave.com.
  speedreader::SetEnabledForSite(
      content_settings(), GURL("https://brave.com/some/nested/page"), false);
  throttle = SpeedReaderThrottle::MaybeCreateThrottleFor(
      nullptr, content_settings(), url(), true, runner);
  EXPECT_EQ(throttle.get(), nullptr);
}

}  // namespace speedreader
