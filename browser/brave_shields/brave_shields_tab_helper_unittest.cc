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
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/favicon/content/content_favicon_driver.h"
#include "components/favicon/core/test/mock_favicon_service.h"
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
  MOCK_METHOD(void, OnAfterRepeatedReloads, (), (override));
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

    local_state_ = std::make_unique<ScopedTestingLocalState>(
        TestingBrowserProcess::GetGlobal());

    favicon::ContentFaviconDriver::CreateForWebContents(web_contents(),
                                                        &favicon_service_);
    BraveShieldsTabHelper::CreateForWebContents(web_contents());
    brave_shields_tab_helper_ =
        BraveShieldsTabHelper::FromWebContents(web_contents());
    observer_ = std::make_unique<
        testing::NiceMock<MockBraveShieldsTabHelperObserver>>();
    brave_shields_tab_helper_->AddObserver(observer_.get());
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

 protected:
  std::unique_ptr<ScopedTestingLocalState> local_state_;
  testing::NiceMock<favicon::MockFaviconService> favicon_service_;
  std::unique_ptr<testing::NiceMock<MockBraveShieldsTabHelperObserver>>
      observer_;
  raw_ptr<BraveShieldsTabHelper> brave_shields_tab_helper_;
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(BraveShieldsTabHelperUnitTest,
       DontTriggerOnAfterRepeatedReloadsWhenFeatureDisabled) {
  feature_list_.InitAndDisableFeature(features::kAdblockOnlyMode);
  SetBraveShieldsAdBlockOnlyModeEnabled(local_state_->Get(), false);

  EXPECT_CALL(*observer_, OnAfterRepeatedReloads).Times(0);

  NavigateTo(GURL("https://example.com"));
  Reload();
  Reload();
  Reload();
}

TEST_F(BraveShieldsTabHelperUnitTest,
       DontTriggerOnAfterRepeatedReloadsWhenAdBlockOnlyModeAlreadyEnabled) {
  feature_list_.InitAndEnableFeatureWithParameters(
      features::kAdblockOnlyMode,
      {
          {features::kAdblockOnlyModeReloadsCountMin.name, "1"},
          {features::kAdblockOnlyModeReloadsCountMax.name, "2"},
          {features::kAdblockOnlyModeReloadsCountInterval.name, "1s"},
      });
  SetBraveShieldsAdBlockOnlyModeEnabled(local_state_->Get(), true);

  EXPECT_CALL(*observer_, OnAfterRepeatedReloads).Times(0);

  NavigateTo(GURL("https://example.com"));
  Reload();
  Reload();
  Reload();
}

TEST_F(BraveShieldsTabHelperUnitTest,
       DontTriggerOnAfterRepeatedReloadsOnNavigates) {
  feature_list_.InitAndEnableFeatureWithParameters(
      features::kAdblockOnlyMode,
      {
          {features::kAdblockOnlyModeReloadsCountMin.name, "1"},
          {features::kAdblockOnlyModeReloadsCountMax.name, "2"},
          {features::kAdblockOnlyModeReloadsCountInterval.name, "1s"},
      });
  SetBraveShieldsAdBlockOnlyModeEnabled(local_state_->Get(), false);

  EXPECT_CALL(*observer_, OnAfterRepeatedReloads).Times(0);

  NavigateTo(GURL("https://example.com"));
  NavigateTo(GURL("https://example.com"));
  NavigateTo(GURL("https://example.com"));
  NavigateTo(GURL("https://example.com"));
  NavigateTo(GURL("https://example.com"));
}

TEST_F(BraveShieldsTabHelperUnitTest,
       DontTriggerOnAfterRepeatedReloadsWhenReloadAfterShieldsEnabledDisabled) {
  feature_list_.InitAndEnableFeatureWithParameters(
      features::kAdblockOnlyMode,
      {
          {features::kAdblockOnlyModeReloadsCountMin.name, "1"},
          {features::kAdblockOnlyModeReloadsCountMax.name, "2"},
          {features::kAdblockOnlyModeReloadsCountInterval.name, "1s"},
      });
  SetBraveShieldsAdBlockOnlyModeEnabled(local_state_->Get(), false);

  EXPECT_CALL(*observer_, OnAfterRepeatedReloads).Times(0);

  brave_shields_tab_helper_->SetBraveShieldsEnabled(true);
  brave_shields_tab_helper_->SetBraveShieldsEnabled(false);
  brave_shields_tab_helper_->SetBraveShieldsEnabled(true);
  brave_shields_tab_helper_->SetBraveShieldsEnabled(false);
}

TEST_F(BraveShieldsTabHelperUnitTest,
       DontTriggerOnAfterRepeatedReloadsByZeroCountFeatureParameters) {
  feature_list_.InitAndEnableFeatureWithParameters(
      features::kAdblockOnlyMode,
      {
          {features::kAdblockOnlyModeReloadsCountMin.name, "0"},
          {features::kAdblockOnlyModeReloadsCountMax.name, "0"},
          {features::kAdblockOnlyModeReloadsCountInterval.name, "1s"},
      });
  SetBraveShieldsAdBlockOnlyModeEnabled(local_state_->Get(), false);

  EXPECT_CALL(*observer_, OnAfterRepeatedReloads).Times(0);

  NavigateTo(GURL("https://example.com"));
  Reload();
  Reload();
  Reload();
  Reload();
  Reload();
}

TEST_F(BraveShieldsTabHelperUnitTest, TriggerOnAfterRepeatedReloads) {
  feature_list_.InitAndEnableFeatureWithParameters(
      features::kAdblockOnlyMode,
      {
          {features::kAdblockOnlyModeReloadsCountMin.name, "1"},
          {features::kAdblockOnlyModeReloadsCountMax.name, "2"},
          {features::kAdblockOnlyModeReloadsCountInterval.name, "1s"},
      });
  SetBraveShieldsAdBlockOnlyModeEnabled(local_state_->Get(), false);

  EXPECT_CALL(*observer_, OnAfterRepeatedReloads).Times(2);

  NavigateTo(GURL("https://example.com"));
  Reload();
  Reload();
  Reload();
  Reload();
  Reload();
  testing::Mock::VerifyAndClearExpectations(observer_.get());

  // Counter is reset after more than `kAdblockOnlyModeReloadsCountInterval`
  // time interval. Fast forward by 2 times to ensure the counter is reset.
  task_environment()->FastForwardBy(
      2 * features::kAdblockOnlyModeReloadsCountInterval.Get());

  EXPECT_CALL(*observer_, OnAfterRepeatedReloads).Times(2);
  Reload();
  Reload();
  Reload();
  Reload();
  Reload();
  testing::Mock::VerifyAndClearExpectations(observer_.get());
}

TEST_F(BraveShieldsTabHelperUnitTest,
       DontTriggerOnAfterRepeatedReloadsWhenPromptDismissed) {
  feature_list_.InitAndEnableFeatureWithParameters(
      features::kAdblockOnlyMode,
      {
          {features::kAdblockOnlyModeReloadsCountMin.name, "1"},
          {features::kAdblockOnlyModeReloadsCountMax.name, "2"},
          {features::kAdblockOnlyModeReloadsCountInterval.name, "1s"},
      });
  SetBraveShieldsAdBlockOnlyModeEnabled(local_state_->Get(), false);
  brave_shields_tab_helper_->SetBraveShieldsAdBlockOnlyModePromptDismissed();

  EXPECT_CALL(*observer_, OnAfterRepeatedReloads).Times(0);

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
  SetBraveShieldsAdBlockOnlyModeEnabled(local_state_->Get(), false);

  // Default value of `features::kAdblockOnlyModeShieldsDisabledCount` is 5.
  brave_shields_tab_helper_->SetBraveShieldsEnabled(false);
  brave_shields_tab_helper_->SetBraveShieldsEnabled(false);
  brave_shields_tab_helper_->SetBraveShieldsEnabled(false);
  brave_shields_tab_helper_->SetBraveShieldsEnabled(false);
  brave_shields_tab_helper_->SetBraveShieldsEnabled(false);

  EXPECT_FALSE(brave_shields_tab_helper_
                   ->ShouldShowShieldsDisabledAdBlockOnlyModePrompt());
}

TEST_F(BraveShieldsTabHelperUnitTest,
       DontShowShieldsDisabledAdBlockOnlyModePromptWhenPromptDismissed) {
  feature_list_.InitAndEnableFeatureWithParameters(
      features::kAdblockOnlyMode,
      {
          {features::kAdblockOnlyModeShieldsDisabledCount.name, "2"},
      });
  SetBraveShieldsAdBlockOnlyModeEnabled(local_state_->Get(), false);
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
          {features::kAdblockOnlyModeShieldsDisabledCount.name, "2"},
      });
  SetBraveShieldsAdBlockOnlyModeEnabled(local_state_->Get(), false);

  brave_shields_tab_helper_->SetBraveShieldsEnabled(false);
  EXPECT_FALSE(brave_shields_tab_helper_
                   ->ShouldShowShieldsDisabledAdBlockOnlyModePrompt());

  brave_shields_tab_helper_->SetBraveShieldsEnabled(false);
  EXPECT_TRUE(brave_shields_tab_helper_
                  ->ShouldShowShieldsDisabledAdBlockOnlyModePrompt());
}

}  // namespace brave_shields
