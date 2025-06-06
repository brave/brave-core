/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "brave/browser/onboarding/onboarding_tab_helper.h"
#include "brave/browser/onboarding/pref_names.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
constexpr char kTestProfileName[] = "TestProfile";
}  // namespace

class OnboardingTest : public testing::Test {
 public:
  OnboardingTest() = default;
  ~OnboardingTest() override = default;

  void SetUp() override {
    TestingBrowserProcess* browser_process = TestingBrowserProcess::GetGlobal();
    profile_manager_ = std::make_unique<TestingProfileManager>(browser_process);
    ASSERT_TRUE(profile_manager_->SetUp());
    profile_ = profile_manager_->CreateTestingProfile(kTestProfileName);
  }

  void TearDown() override {
    profile_ = nullptr;
    profile_manager_->DeleteTestingProfile(kTestProfileName);
  }

  void MaybeCreateForWebContentsAndWaitTillGetResult(
      content::WebContents* web_contents) {
    bool created = false;
    web_contents->SetUserData(OnboardingTabHelper::UserDataKey(), nullptr);
    OnboardingTabHelper::MaybeCreateForWebContents(
        web_contents,
        base::BindLambdaForTesting([&created] { created = true; }));
    ASSERT_TRUE(base::test::RunUntil([&created] { return created; }));
  }

  Profile* profile() { return profile_; }

  content::BrowserTaskEnvironment task_environment_;
  content::RenderViewHostTestEnabler render_view_host_test_enabler_;
  raw_ptr<Profile> profile_ = nullptr;
  std::unique_ptr<TestingProfileManager> profile_manager_;
};

TEST_F(OnboardingTest, HelperCreationTestForFirstRun) {
  first_run::ResetCachedSentinelDataForTesting();
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kForceFirstRun);
  ASSERT_TRUE(first_run::IsChromeFirstRun());

  auto web_contents =
      content::WebContentsTester::CreateTestWebContents(profile(), nullptr);
  ASSERT_TRUE(web_contents);

  // Check helper is created for first run.
  MaybeCreateForWebContentsAndWaitTillGetResult(web_contents.get());
  auto* tab_helper = OnboardingTabHelper::FromWebContents(web_contents.get());
  EXPECT_TRUE(tab_helper);

  // Even seven days passed durig the first run, helper should be created.
  OnboardingTabHelper::s_time_now_for_testing_ =
      base::Time::Now() + base::Days(8);
  MaybeCreateForWebContentsAndWaitTillGetResult(web_contents.get());
  tab_helper = OnboardingTabHelper::FromWebContents(web_contents.get());
  EXPECT_TRUE(tab_helper);

  // Check helper is not created when |kLastShieldsIconHighlightTime| is not
  // null.
  TestingBrowserProcess::GetGlobal()->local_state()->SetTime(
      onboarding::prefs::kLastShieldsIconHighlightTime, base::Time::Now());
  MaybeCreateForWebContentsAndWaitTillGetResult(web_contents.get());
  tab_helper = OnboardingTabHelper::FromWebContents(web_contents.get());
  EXPECT_FALSE(tab_helper);
}

TEST_F(OnboardingTest, HelperCreationTestForNonFirstRun) {
  // Create sentinel as OnboardingTabHelper::IsSevenDaysPassedSinceFirstRun()
  // checks its creation time to know how long it's been since then.
  OnboardingTabHelper::s_time_now_for_testing_ = base::Time::Now();
  OnboardingTabHelper::s_sentinel_time_for_testing_ = base::Time::Now();
  first_run::ResetCachedSentinelDataForTesting();
  base::CommandLine::ForCurrentProcess()->AppendSwitch(switches::kNoFirstRun);
  ASSERT_FALSE(first_run::IsChromeFirstRun());

  auto web_contents =
      content::WebContentsTester::CreateTestWebContents(profile(), nullptr);
  ASSERT_TRUE(web_contents);

  // Check helper is not created when |kLastShieldsIconHighlightTime| is not
  // null.
  TestingBrowserProcess::GetGlobal()->local_state()->SetTime(
      onboarding::prefs::kLastShieldsIconHighlightTime, base::Time::Now());
  MaybeCreateForWebContentsAndWaitTillGetResult(web_contents.get());
  auto* tab_helper = OnboardingTabHelper::FromWebContents(web_contents.get());
  EXPECT_FALSE(tab_helper);

  // Check helper is created when |kLastShieldsIconHighlightTime| is null.
  TestingBrowserProcess::GetGlobal()->local_state()->ClearPref(
      onboarding::prefs::kLastShieldsIconHighlightTime);
  MaybeCreateForWebContentsAndWaitTillGetResult(web_contents.get());
  tab_helper = OnboardingTabHelper::FromWebContents(web_contents.get());
  ASSERT_TRUE(tab_helper);
  ASSERT_TRUE(tab_helper->CanHighlightBraveShields());

  // Check exiting tab doesn't give highlight when 7 days passed.
  OnboardingTabHelper::s_time_now_for_testing_ =
      base::Time::Now() + base::Days(7);
  EXPECT_FALSE(tab_helper->CanHighlightBraveShields());
}
