/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ntp_background/new_tab_takeover_infobar_delegate.h"

#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_feature.h"
#include "brave/components/ntp_background_images/common/new_tab_takeover_infobar_util.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "components/infobars/core/infobar_delegate.h"
#include "components/prefs/pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#endif  // !BUILDFLAG(IS_ANDROID)

namespace ntp_background_images {

class NewTabTakeoverInfoBarDelegateTest
    : public ChromeRenderViewHostTestHarness {
 public:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

    infobars::ContentInfoBarManager::CreateForWebContents(web_contents());
  }

  infobars::ContentInfoBarManager* GetInfobarManager() {
    return infobars::ContentInfoBarManager::FromWebContents(web_contents());
  }

  PrefService* GetPrefs() { return profile()->GetPrefs(); }

  void CreateInfobar() {
    EXPECT_THAT(GetInfobarManager()->infobars().size(), ::testing::Eq(0));
    NewTabTakeoverInfoBarDelegate::MaybeCreate(web_contents(), GetPrefs());
    EXPECT_THAT(GetInfobarManager()->infobars().size(), ::testing::Eq(1));
  }

  void CloseInfobar() {
    GetInfobarManager()->RemoveAllInfoBars(false);
    EXPECT_THAT(GetInfobarManager()->infobars().size(), ::testing::Eq(0));
  }

  void VerifyInfobarCannotBeCreated() {
    NewTabTakeoverInfoBarDelegate::MaybeCreate(web_contents(), GetPrefs());
    EXPECT_THAT(GetInfobarManager()->infobars().size(), ::testing::Eq(0));
  }

  void EnableSupportNewTabPageAdConfirmationsForNonRewards() {
    scoped_feature_list_.InitWithFeaturesAndParameters(
        {{brave_ads::kNewTabPageAdFeature,
          {{"should_support_confirmations_for_non_rewards", "true"}}}},
        {});
  }

  void DisableSupportNewTabPageAdConfirmationsForNonRewards() {
    scoped_feature_list_.InitWithFeaturesAndParameters(
        {{brave_ads::kNewTabPageAdFeature,
          {{"should_support_confirmations_for_non_rewards", "false"}}}},
        {});
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

#if !BUILDFLAG(IS_ANDROID)
  ChromeLayoutProvider layout_provider_;
#endif  // !BUILDFLAG(IS_ANDROID)
};

TEST_F(NewTabTakeoverInfoBarDelegateTest,
       InfobarNotCreatedWhenSupportNewTabPageAdConfirmationsForNonRewardsDisabled) {
  DisableSupportNewTabPageAdConfirmationsForNonRewards();

  EXPECT_THAT(GetPrefs()->GetInteger(prefs::kNewTabTakeoverInfobarShowCount),
              ::testing::Eq(GetNewTabTakeoverInfobarShowCountThreshold()));

  VerifyInfobarCannotBeCreated();
}

TEST_F(NewTabTakeoverInfoBarDelegateTest, CreateInfobar) {
  EnableSupportNewTabPageAdConfirmationsForNonRewards();

  EXPECT_THAT(GetPrefs()->GetInteger(prefs::kNewTabTakeoverInfobarShowCount),
              ::testing::Eq(GetNewTabTakeoverInfobarShowCountThreshold()));

  CreateInfobar();

  EXPECT_THAT(GetPrefs()->GetInteger(prefs::kNewTabTakeoverInfobarShowCount),
              ::testing::Eq(GetNewTabTakeoverInfobarShowCountThreshold() - 1));
}

TEST_F(NewTabTakeoverInfoBarDelegateTest,
       DisplayNewTabTakeoverInfobarUntilThreshold) {
  EnableSupportNewTabPageAdConfirmationsForNonRewards();

  EXPECT_THAT(GetPrefs()->GetInteger(prefs::kNewTabTakeoverInfobarShowCount),
              ::testing::Eq(GetNewTabTakeoverInfobarShowCountThreshold()));

  for (int i = 0; i < GetNewTabTakeoverInfobarShowCountThreshold(); i++) {
    CreateInfobar();
    CloseInfobar();
  }

  EXPECT_THAT(GetPrefs()->GetInteger(prefs::kNewTabTakeoverInfobarShowCount),
              ::testing::Eq(0));

  VerifyInfobarCannotBeCreated();
}

TEST_F(NewTabTakeoverInfoBarDelegateTest,
       DontDisplayNewTabTakeoverInfobarWhenThresholdIsReached) {
  EnableSupportNewTabPageAdConfirmationsForNonRewards();

  GetPrefs()->SetInteger(prefs::kNewTabTakeoverInfobarShowCount, 0);

  VerifyInfobarCannotBeCreated();
}

TEST_F(NewTabTakeoverInfoBarDelegateTest,
       DontDisplayNewTabTakeoverInfobarAfterInfobarIsDismissed) {
  EnableSupportNewTabPageAdConfirmationsForNonRewards();

  CreateInfobar();
  ASSERT_THAT(GetInfobarManager()->infobars().size(), ::testing::Eq(1));

  infobars::InfoBar* infobar = GetInfobarManager()->infobars()[0];
  infobar->delegate()->InfoBarDismissed();
  CloseInfobar();

  VerifyInfobarCannotBeCreated();
}

TEST_F(NewTabTakeoverInfoBarDelegateTest,
       DontDisplayNewTabTakeoverInfobarAfterLinkIsClicked) {
  EnableSupportNewTabPageAdConfirmationsForNonRewards();

  CreateInfobar();
  ASSERT_THAT(GetInfobarManager()->infobars().size(), ::testing::Eq(1));

  infobars::InfoBar* infobar = GetInfobarManager()->infobars()[0];
  infobar->delegate()->LinkClicked(WindowOpenDisposition::CURRENT_TAB);
  CloseInfobar();

  VerifyInfobarCannotBeCreated();
}

}  // namespace ntp_background_images
