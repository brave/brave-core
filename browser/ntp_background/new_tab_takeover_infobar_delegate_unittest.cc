/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ntp_background/new_tab_takeover_infobar_delegate.h"

#include "brave/components/ntp_background_images/common/new_tab_takeover_infobar_util.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/infobars/content/content_infobar_manager.h"
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

 private:
#if !BUILDFLAG(IS_ANDROID)
  ChromeLayoutProvider layout_provider_;
#endif  // !BUILDFLAG(IS_ANDROID)
};

TEST_F(NewTabTakeoverInfoBarDelegateTest, CreateInfobar) {
  EXPECT_THAT(GetInfobarManager()->infobars().size(), ::testing::Eq(0));
  ntp_background_images::NewTabTakeoverInfoBarDelegate::MaybeCreate(
      web_contents(), profile()->GetPrefs());
  EXPECT_THAT(GetInfobarManager()->infobars().size(), ::testing::Eq(1));
}

TEST_F(NewTabTakeoverInfoBarDelegateTest,
       ShowNewTabTakeoverInfobarUntilThreshold) {
  for (int i = 0; i < GetNewTabTakeoverInfobarShowCountThreshold(); i++) {
    ntp_background_images::NewTabTakeoverInfoBarDelegate::MaybeCreate(
        web_contents(), profile()->GetPrefs());
    EXPECT_THAT(GetInfobarManager()->infobars().size(), ::testing::Eq(1));
    GetInfobarManager()->RemoveAllInfoBars(false);
    EXPECT_THAT(GetInfobarManager()->infobars().size(), ::testing::Eq(0));
  }

  ntp_background_images::NewTabTakeoverInfoBarDelegate::MaybeCreate(
      web_contents(), profile()->GetPrefs());
  EXPECT_THAT(GetInfobarManager()->infobars().size(), ::testing::Eq(0));
}

}  // namespace ntp_background_images
