/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ntp_background/new_tab_takeover_infobar_delegate.h"

#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/ntp_background_images/browser/new_tab_takeover_infobar_util.h"
#include "brave/components/ntp_background_images/common/infobar_constants.h"
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
    EXPECT_THAT(GetInfobarManager()->infobars(), ::testing::IsEmpty());
    NewTabTakeoverInfoBarDelegate::MaybeDisplayAndIncrementCounter(
        web_contents(), GetPrefs());
    EXPECT_THAT(GetInfobarManager()->infobars(), ::testing::SizeIs(1));
  }

  void CloseInfobar() {
    GetInfobarManager()->RemoveAllInfoBars(/*animate=*/false);
    EXPECT_THAT(GetInfobarManager()->infobars(), ::testing::IsEmpty());
  }

  void VerifyInfobarWasNotDisplayedExpectation() {
    NewTabTakeoverInfoBarDelegate::MaybeDisplayAndIncrementCounter(
        web_contents(), GetPrefs());
    EXPECT_THAT(GetInfobarManager()->infobars(), ::testing::IsEmpty());
  }

  void SetRewardsEnabled(bool enabled) {
    GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, enabled);
  }

 private:
#if !BUILDFLAG(IS_ANDROID)
  ChromeLayoutProvider layout_provider_;
#endif  // !BUILDFLAG(IS_ANDROID)
};

TEST_F(NewTabTakeoverInfoBarDelegateTest,
       ShouldDisplayInfobarIfShouldSupportConfirmationsForNonRewards) {
  SetRewardsEnabled(/*enabled=*/false);

  EXPECT_THAT(
      GetPrefs()->GetInteger(
          prefs::kNewTabTakeoverInfobarRemainingDisplayCount),
      ::testing::Eq(kNewTabTakeoverInfobarRemainingDisplayCountThreshold));

  CreateInfobar();

  EXPECT_THAT(
      GetPrefs()->GetInteger(
          prefs::kNewTabTakeoverInfobarRemainingDisplayCount),
      ::testing::Eq(kNewTabTakeoverInfobarRemainingDisplayCountThreshold - 1));
}

TEST_F(NewTabTakeoverInfoBarDelegateTest,
       ShouldNotDisplayInfobarIfShouldSupportConfirmationsForRewards) {
  SetRewardsEnabled(/*enabled=*/true);

  EXPECT_THAT(
      GetPrefs()->GetInteger(
          prefs::kNewTabTakeoverInfobarRemainingDisplayCount),
      ::testing::Eq(kNewTabTakeoverInfobarRemainingDisplayCountThreshold));

  VerifyInfobarWasNotDisplayedExpectation();
}

TEST_F(NewTabTakeoverInfoBarDelegateTest,
       ShouldDisplayInfobarWhenThresholdHasNotBeenExceeded) {
  SetRewardsEnabled(/*enabled=*/false);

  EXPECT_THAT(
      GetPrefs()->GetInteger(
          prefs::kNewTabTakeoverInfobarRemainingDisplayCount),
      ::testing::Eq(kNewTabTakeoverInfobarRemainingDisplayCountThreshold));

  for (int i = 0; i < kNewTabTakeoverInfobarRemainingDisplayCountThreshold;
       i++) {
    CreateInfobar();
    CloseInfobar();
  }

  EXPECT_THAT(GetPrefs()->GetInteger(
                  prefs::kNewTabTakeoverInfobarRemainingDisplayCount),
              ::testing::Eq(0));

  VerifyInfobarWasNotDisplayedExpectation();
}

TEST_F(NewTabTakeoverInfoBarDelegateTest,
       ShouldNotDisplayInfobarWhenThresholdIsMet) {
  SetRewardsEnabled(/*enabled=*/false);

  GetPrefs()->SetInteger(prefs::kNewTabTakeoverInfobarRemainingDisplayCount, 0);

  VerifyInfobarWasNotDisplayedExpectation();
}

TEST_F(NewTabTakeoverInfoBarDelegateTest,
       ShouldNotDisplayInfobarWhenThresholdIsExceeded) {
  SetRewardsEnabled(/*enabled=*/false);

  GetPrefs()->SetInteger(prefs::kNewTabTakeoverInfobarRemainingDisplayCount,
                         -1);

  VerifyInfobarWasNotDisplayedExpectation();
}

TEST_F(NewTabTakeoverInfoBarDelegateTest,
       ShouldNeverDisplayInfobarAgainIfClosedByUser) {
  SetRewardsEnabled(/*enabled=*/false);

  CreateInfobar();
  ASSERT_THAT(GetInfobarManager()->infobars(), ::testing::SizeIs(1));

  infobars::InfoBar* infobar = GetInfobarManager()->infobars()[0];
  infobar->delegate()->InfoBarDismissed();
  CloseInfobar();

  VerifyInfobarWasNotDisplayedExpectation();
}

TEST_F(NewTabTakeoverInfoBarDelegateTest,
       ShouldNeverDisplayInfobarAgainIfUserClicksLearnMoreLink) {
  SetRewardsEnabled(/*enabled=*/false);

  CreateInfobar();
  ASSERT_THAT(GetInfobarManager()->infobars(), ::testing::SizeIs(1));

  infobars::InfoBar* infobar = GetInfobarManager()->infobars()[0];
  infobar->delegate()->LinkClicked(WindowOpenDisposition::CURRENT_TAB);
  CloseInfobar();

  VerifyInfobarWasNotDisplayedExpectation();
}

}  // namespace ntp_background_images
