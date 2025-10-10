// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_shields/brave_shields_tab_helper.h"

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_shields/core/common/brave_shield_utils.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/favicon/content/content_favicon_driver.h"
#include "components/favicon/core/test/mock_favicon_service.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "content/test/test_web_contents.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_shields {

namespace {

class MockBraveShieldsTabHelperObserver
    : public BraveShieldsTabHelper::Observer {
 public:
  MOCK_METHOD(void, OnResourcesChanged, (), (override));
  MOCK_METHOD(void, OnFaviconUpdated, (), (override));
  MOCK_METHOD(void, OnShieldsEnabledChanged, (), (override));
  MOCK_METHOD(void, OnShieldsAdBlockOnlyModeEnabledChanged, (), (override));
  MOCK_METHOD(void, OnRepeatedReloadsDetected, (), (override));
};

}  // namespace

class BraveShieldsTabHelperUnitTest
    : public content::RenderViewHostTestHarness {
 public:
  BraveShieldsTabHelperUnitTest()
      : content::RenderViewHostTestHarness(
            base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  ~BraveShieldsTabHelperUnitTest() override = default;

  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();

    favicon::ContentFaviconDriver::CreateForWebContents(web_contents(),
                                                        &favicon_service_);
    BraveShieldsTabHelper::CreateForWebContents(web_contents());
    brave_shields_tab_helper_ =
        BraveShieldsTabHelper::FromWebContents(web_contents());
    observer_ = std::make_unique<
        testing::NiceMock<MockBraveShieldsTabHelperObserver>>();
    brave_shields_tab_helper_->AddObserver(observer_.get());

    TestingBrowserProcess::GetGlobal()->SetApplicationLocale("en-US");
  }

  void TearDown() override {
    brave_shields_tab_helper_->RemoveObserver(observer_.get());
    brave_shields_tab_helper_ = nullptr;
    content::RenderViewHostTestHarness::TearDown();
  }

  std::unique_ptr<content::BrowserContext> CreateBrowserContext() override {
    return std::make_unique<TestingProfile>();
  }

  void NavigateTo(GURL url) {
    content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                               url);
    EXPECT_EQ(web_contents()->GetLastCommittedURL(), url);
  }

  void Reload() { content::NavigationSimulator::Reload(web_contents()); }

  PrefService* profile_prefs() {
    return Profile::FromBrowserContext(web_contents()->GetBrowserContext())
        ->GetPrefs();
  }

  void SetApplicationLocale(const std::string& locale) {
    TestingBrowserProcess::GetGlobal()->SetApplicationLocale(locale);
  }

  PrefService* local_state() {
    return TestingBrowserProcess::GetGlobal()->GetTestingLocalState();
  }

 protected:
  testing::NiceMock<favicon::MockFaviconService> favicon_service_;
  std::unique_ptr<testing::NiceMock<MockBraveShieldsTabHelperObserver>>
      observer_;
  raw_ptr<BraveShieldsTabHelper> brave_shields_tab_helper_;
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(BraveShieldsTabHelperUnitTest,
       DontTriggerOnRepeatedReloadsDetectedWhenFeatureDisabled) {
  feature_list_.InitAndDisableFeature(features::kAdblockOnlyMode);
  SetBraveShieldsAdBlockOnlyModeEnabled(local_state(), false);

  EXPECT_CALL(*observer_, OnRepeatedReloadsDetected).Times(0);

  NavigateTo(GURL("https://example.com"));
  Reload();
  Reload();
  Reload();
}

TEST_F(BraveShieldsTabHelperUnitTest,
       DontTriggerOnRepeatedReloadsDetectedWhenLocaleNotSupported) {
  SetApplicationLocale("fr-FR");
  SetBraveShieldsAdBlockOnlyModeEnabled(local_state(), false);

  EXPECT_CALL(*observer_, OnRepeatedReloadsDetected).Times(0);

  NavigateTo(GURL("https://example.com"));
  Reload();
  Reload();
  Reload();
}

TEST_F(BraveShieldsTabHelperUnitTest,
       DontTriggerOnRepeatedReloadsDetectedWhenAdBlockOnlyModeAlreadyEnabled) {
  feature_list_.InitAndEnableFeatureWithParameters(
      features::kAdblockOnlyMode,
      {
          {features::kAdblockOnlyModePromptAfterPageReloadsMin.name, "1"},
          {features::kAdblockOnlyModePromptAfterPageReloadsMax.name, "2"},
          {features::kAdblockOnlyModePromptAfterPageReloadsInterval.name, "1s"},
      });
  SetBraveShieldsAdBlockOnlyModeEnabled(local_state(), true);

  EXPECT_CALL(*observer_, OnRepeatedReloadsDetected).Times(0);

  NavigateTo(GURL("https://example.com"));
  Reload();
  Reload();
  Reload();
}

TEST_F(BraveShieldsTabHelperUnitTest,
       DontTriggerOnRepeatedReloadsDetectedOnNotReloadNavigation) {
  feature_list_.InitAndEnableFeatureWithParameters(
      features::kAdblockOnlyMode,
      {
          {features::kAdblockOnlyModePromptAfterPageReloadsMin.name, "1"},
          {features::kAdblockOnlyModePromptAfterPageReloadsMax.name, "2"},
          {features::kAdblockOnlyModePromptAfterPageReloadsInterval.name, "1s"},
      });
  SetBraveShieldsAdBlockOnlyModeEnabled(local_state(), false);

  EXPECT_CALL(*observer_, OnRepeatedReloadsDetected).Times(0);

  NavigateTo(GURL("https://example.com"));
  NavigateTo(GURL("https://example.com"));
  NavigateTo(GURL("https://example.com"));
  NavigateTo(GURL("https://example.com"));
  NavigateTo(GURL("https://example.com"));
}

TEST_F(BraveShieldsTabHelperUnitTest,
       DontTriggerOnRepeatedReloadsDetectedWhenReloadsByShieldsChanges) {
  feature_list_.InitAndEnableFeatureWithParameters(
      features::kAdblockOnlyMode,
      {
          {features::kAdblockOnlyModePromptAfterPageReloadsMin.name, "1"},
          {features::kAdblockOnlyModePromptAfterPageReloadsMax.name, "2"},
          {features::kAdblockOnlyModePromptAfterPageReloadsInterval.name, "1s"},
      });
  SetBraveShieldsAdBlockOnlyModeEnabled(local_state(), false);

  EXPECT_CALL(*observer_, OnRepeatedReloadsDetected).Times(0);

  brave_shields_tab_helper_->SetBraveShieldsEnabled(true);
  brave_shields_tab_helper_->SetBraveShieldsEnabled(false);
  brave_shields_tab_helper_->SetBraveShieldsEnabled(true);
  brave_shields_tab_helper_->SetBraveShieldsEnabled(false);
}

TEST_F(BraveShieldsTabHelperUnitTest,
       DontTriggerOnRepeatedReloadsDetectedByZeroCountFeatureParameters) {
  feature_list_.InitAndEnableFeatureWithParameters(
      features::kAdblockOnlyMode,
      {
          {features::kAdblockOnlyModePromptAfterPageReloadsMin.name, "0"},
          {features::kAdblockOnlyModePromptAfterPageReloadsMax.name, "0"},
          {features::kAdblockOnlyModePromptAfterPageReloadsInterval.name, "1s"},
      });
  SetBraveShieldsAdBlockOnlyModeEnabled(local_state(), false);

  EXPECT_CALL(*observer_, OnRepeatedReloadsDetected).Times(0);

  NavigateTo(GURL("https://example.com"));
  Reload();
  Reload();
  Reload();
  Reload();
  Reload();
}

TEST_F(BraveShieldsTabHelperUnitTest, TriggerOnRepeatedReloadsDetected) {
  feature_list_.InitAndEnableFeatureWithParameters(
      features::kAdblockOnlyMode,
      {
          {features::kAdblockOnlyModePromptAfterPageReloadsMin.name, "1"},
          {features::kAdblockOnlyModePromptAfterPageReloadsMax.name, "2"},
          {features::kAdblockOnlyModePromptAfterPageReloadsInterval.name, "1s"},
      });
  SetBraveShieldsAdBlockOnlyModeEnabled(local_state(), false);

  EXPECT_CALL(*observer_, OnRepeatedReloadsDetected).Times(2);

  NavigateTo(GURL("https://example.com"));
  Reload();
  Reload();
  Reload();
  Reload();
  Reload();
  testing::Mock::VerifyAndClearExpectations(observer_.get());

  // Counter is reset after more than
  // `kAdblockOnlyModePromptAfterPageReloadsInterval` time interval. Fast
  // forward by 2 times to ensure the counter is reset.
  task_environment()->FastForwardBy(
      2 * features::kAdblockOnlyModePromptAfterPageReloadsInterval.Get());

  EXPECT_CALL(*observer_, OnRepeatedReloadsDetected).Times(2);
  Reload();
  Reload();
  Reload();
  Reload();
  Reload();
  testing::Mock::VerifyAndClearExpectations(observer_.get());
}

TEST_F(BraveShieldsTabHelperUnitTest,
       DontTriggerOnRepeatedReloadsDetectedWhenPromptDismissed) {
  feature_list_.InitAndEnableFeatureWithParameters(
      features::kAdblockOnlyMode,
      {
          {features::kAdblockOnlyModePromptAfterPageReloadsMin.name, "1"},
          {features::kAdblockOnlyModePromptAfterPageReloadsMax.name, "2"},
          {features::kAdblockOnlyModePromptAfterPageReloadsInterval.name, "1s"},
      });
  SetBraveShieldsAdBlockOnlyModeEnabled(local_state(), false);
  brave_shields_tab_helper_->SetBraveShieldsAdBlockOnlyModePromptDismissed();

  EXPECT_CALL(*observer_, OnRepeatedReloadsDetected).Times(0);

  NavigateTo(GURL("https://example.com"));
  Reload();
  Reload();
  Reload();
  Reload();
  Reload();
}

TEST_F(BraveShieldsTabHelperUnitTest,
       DontShowShieldsDisabledAdBlockOnlyModePromptWhenFeatureDisabled) {
  feature_list_.InitAndDisableFeature(features::kAdblockOnlyMode);
  SetBraveShieldsAdBlockOnlyModeEnabled(local_state(), false);

  // Default value of
  // `features::kAdblockOnlyModePromptAfterShieldsDisabledCount` is 5.
  brave_shields_tab_helper_->SetBraveShieldsEnabled(false);
  brave_shields_tab_helper_->SetBraveShieldsEnabled(false);
  brave_shields_tab_helper_->SetBraveShieldsEnabled(false);
  brave_shields_tab_helper_->SetBraveShieldsEnabled(false);
  brave_shields_tab_helper_->SetBraveShieldsEnabled(false);

  EXPECT_EQ(
      profile_prefs()->GetInteger(brave_shields::prefs::kShieldsDisabledCount),
      0);

  EXPECT_FALSE(brave_shields_tab_helper_
                   ->ShouldShowShieldsDisabledAdBlockOnlyModePrompt());
}

TEST_F(BraveShieldsTabHelperUnitTest,
       DontShowShieldsDisabledAdBlockOnlyModePromptWhenLocaleNotSupported) {
  SetApplicationLocale("fr-FR");
  SetBraveShieldsAdBlockOnlyModeEnabled(local_state(), false);

  // Default value of
  // `features::kAdblockOnlyModePromptAfterShieldsDisabledCount` is 5.
  brave_shields_tab_helper_->SetBraveShieldsEnabled(false);
  brave_shields_tab_helper_->SetBraveShieldsEnabled(false);
  brave_shields_tab_helper_->SetBraveShieldsEnabled(false);
  brave_shields_tab_helper_->SetBraveShieldsEnabled(false);
  brave_shields_tab_helper_->SetBraveShieldsEnabled(false);

  EXPECT_EQ(
      profile_prefs()->GetInteger(brave_shields::prefs::kShieldsDisabledCount),
      0);

  EXPECT_FALSE(brave_shields_tab_helper_
                   ->ShouldShowShieldsDisabledAdBlockOnlyModePrompt());
}

TEST_F(BraveShieldsTabHelperUnitTest,
       DontShowShieldsDisabledAdBlockOnlyModePromptWhenPromptDismissed) {
  feature_list_.InitAndEnableFeatureWithParameters(
      features::kAdblockOnlyMode,
      {
          {features::kAdblockOnlyModePromptAfterShieldsDisabledCount.name, "2"},
      });
  SetBraveShieldsAdBlockOnlyModeEnabled(local_state(), false);
  brave_shields_tab_helper_->SetBraveShieldsAdBlockOnlyModePromptDismissed();

  brave_shields_tab_helper_->SetBraveShieldsEnabled(false);
  brave_shields_tab_helper_->SetBraveShieldsEnabled(false);
  EXPECT_FALSE(brave_shields_tab_helper_
                   ->ShouldShowShieldsDisabledAdBlockOnlyModePrompt());
}

TEST_F(BraveShieldsTabHelperUnitTest,
       ShowShieldsDisabledAdBlockOnlyModePrompt) {
  feature_list_.InitAndEnableFeatureWithParameters(
      features::kAdblockOnlyMode,
      {
          {features::kAdblockOnlyModePromptAfterShieldsDisabledCount.name, "2"},
      });
  SetBraveShieldsAdBlockOnlyModeEnabled(local_state(), false);

  brave_shields_tab_helper_->SetBraveShieldsEnabled(false);
  EXPECT_FALSE(brave_shields_tab_helper_
                   ->ShouldShowShieldsDisabledAdBlockOnlyModePrompt());

  brave_shields_tab_helper_->SetBraveShieldsEnabled(false);
  EXPECT_TRUE(brave_shields_tab_helper_
                  ->ShouldShowShieldsDisabledAdBlockOnlyModePrompt());
}

}  // namespace brave_shields
