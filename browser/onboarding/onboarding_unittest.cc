/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/test/run_until.h"
#include "base/test/scoped_path_override.h"
#include "base/time/time.h"
#include "base/time/time_override.h"
#include "brave/browser/onboarding/onboarding_tab_helper.h"
#include "brave/browser/onboarding/pref_names.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/prefs/pref_service.h"
#include "components/tabs/public/mock_tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr char kTestProfileName[] = "TestProfile";

// What base::Time::Now() returns while a ScopedTimeClockOverrides uses
// FakeTimeNow().
base::Time g_fake_time_now;

base::Time FakeTimeNow() {
  return g_fake_time_now;
}

}  // namespace

class OnboardingTest : public testing::Test {
 public:
  OnboardingTest() = default;
  ~OnboardingTest() override = default;

  void SetUp() override {
    first_run::ResetCachedSentinelDataForTesting();
    TestingBrowserProcess* browser_process = TestingBrowserProcess::GetGlobal();
    profile_manager_ = std::make_unique<TestingProfileManager>(browser_process);
    ASSERT_TRUE(profile_manager_->SetUp());
    profile_ = profile_manager_->CreateTestingProfile(kTestProfileName);
    web_contents_ =
        content::WebContentsTester::CreateTestWebContents(profile_, nullptr);
    ON_CALL(tab_, GetContents())
        .WillByDefault(testing::Return(web_contents_.get()));
  }

  void TearDown() override {
    web_contents_.reset();
    profile_ = nullptr;
    profile_manager_->DeleteTestingProfile(kTestProfileName);
    first_run::ResetCachedSentinelDataForTesting();
  }

  // Creates the first run sentinel, which dates Brave's install to now.
  void CreateSentinel() {
    ASSERT_TRUE(
        base::WriteFile(base::PathService::CheckedGet(chrome::DIR_USER_DATA)
                            .Append(chrome::kFirstRunSentinel),
                        ""));
  }

  // Makes base::Time::Now() return `delta` from now, until the returned value
  // is destroyed.
  std::unique_ptr<base::subtle::ScopedTimeClockOverrides> AdvanceTimeNow(
      base::TimeDelta delta) {
    g_fake_time_now = base::Time::Now() + delta;
    return std::make_unique<base::subtle::ScopedTimeClockOverrides>(
        &FakeTimeNow, nullptr, nullptr);
  }

  // Creates the helper and waits for its first-run check, which runs on the
  // thread pool on non-first runs.
  std::unique_ptr<OnboardingTabHelper> MaybeCreateAndWait() {
    auto helper = OnboardingTabHelper::MaybeCreate(tab_);
    if (helper) {
      EXPECT_TRUE(base::test::RunUntil(
          [&] { return helper->can_highlight_ || helper->cleaned_up_; }));
    }
    return helper;
  }

  base::ScopedPathOverride user_data_dir_override_{chrome::DIR_USER_DATA};
  content::BrowserTaskEnvironment task_environment_;
  content::RenderViewHostTestEnabler render_view_host_test_enabler_;
  raw_ptr<Profile> profile_ = nullptr;
  std::unique_ptr<TestingProfileManager> profile_manager_;
  std::unique_ptr<content::WebContents> web_contents_;
  tabs::MockTabInterface tab_;
};

TEST_F(OnboardingTest, HelperCreationTestForFirstRun) {
  first_run::ResetCachedSentinelDataForTesting();
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kForceFirstRun);
  ASSERT_TRUE(first_run::IsChromeFirstRun());

  // Check helper is created for first run.
  auto tab_helper = MaybeCreateAndWait();
  ASSERT_TRUE(tab_helper);
  EXPECT_TRUE(tab_helper->can_highlight_);

  // Even seven days passed durig the first run, helper should be created.
  const auto time_override = AdvanceTimeNow(base::Days(8));
  tab_helper = MaybeCreateAndWait();
  ASSERT_TRUE(tab_helper);
  EXPECT_TRUE(tab_helper->can_highlight_);

  // Check helper is not created when |kLastShieldsIconHighlightTime| is not
  // null.
  TestingBrowserProcess::GetGlobal()->local_state()->SetTime(
      onboarding::prefs::kLastShieldsIconHighlightTime, base::Time::Now());
  EXPECT_FALSE(MaybeCreateAndWait());
}

TEST_F(OnboardingTest, HelperCreationTestForNonFirstRun) {
  // OnboardingTabHelper::IsSevenDaysPassedSinceFirstRun() checks the
  // sentinel's creation time to know how long it's been since then.
  ASSERT_NO_FATAL_FAILURE(CreateSentinel());
  first_run::ResetCachedSentinelDataForTesting();
  base::CommandLine::ForCurrentProcess()->AppendSwitch(switches::kNoFirstRun);
  ASSERT_FALSE(first_run::IsChromeFirstRun());

  // Check helper is not created when |kLastShieldsIconHighlightTime| is not
  // null.
  TestingBrowserProcess::GetGlobal()->local_state()->SetTime(
      onboarding::prefs::kLastShieldsIconHighlightTime, base::Time::Now());
  EXPECT_FALSE(MaybeCreateAndWait());

  // Check helper is created when |kLastShieldsIconHighlightTime| is null.
  TestingBrowserProcess::GetGlobal()->local_state()->ClearPref(
      onboarding::prefs::kLastShieldsIconHighlightTime);
  auto tab_helper = MaybeCreateAndWait();
  ASSERT_TRUE(tab_helper);
  EXPECT_TRUE(tab_helper->can_highlight_);
  ASSERT_TRUE(tab_helper->CanHighlightBraveShields());

  // Check exiting tab doesn't give highlight when 7 days passed.
  const auto time_override = AdvanceTimeNow(base::Days(8));
  EXPECT_FALSE(tab_helper->CanHighlightBraveShields());

  // Check a new tab stops observing once the check finds 7 days passed.
  tab_helper = MaybeCreateAndWait();
  ASSERT_TRUE(tab_helper);
  EXPECT_FALSE(tab_helper->can_highlight_);
  EXPECT_FALSE(tab_helper->web_contents());
}
