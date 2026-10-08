/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "base/test/test_future.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_test_util.h"
#include "chrome/browser/signin/signin_promo_util.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/ui/views/profiles/avatar_toolbar_button.h"
#include "chrome/grit/branded_strings.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "components/signin/public/base/signin_pref_names.h"
#include "components/signin/public/base/signin_switches.h"
#include "content/public/test/browser_test.h"
#include "ui/base/l10n/l10n_util.h"

// Brave doesn't offer browser sign-in to Google, but kSigninAllowed is on for
// users who allow Google login for extensions. Upstream must never show its
// avatar button promos to them.
class BraveAvatarToolbarButtonBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpDefaultCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpDefaultCommandLine(command_line);
    command_line->RemoveSwitch(
        switches::kDisableSigninPromoOnAvatarPillForTesting);
  }

  AvatarToolbarButton* GetAvatarButton() {
    return static_cast<AvatarToolbarButton*>(
        BrowserView::GetBrowserViewForBrowser(browser())
            ->toolbar_button_provider()
            ->GetAvatarToolbarButtonInterface());
  }
};

IN_PROC_BROWSER_TEST_F(BraveAvatarToolbarButtonBrowserTest,
                       PRE_NoSignInPromoWhenSigninAllowed) {
  browser()->GetProfile()->GetPrefs()->SetBoolean(
      prefs::kSigninAllowedOnNextStartup, true);

  // Brave only shows the avatar button when there are several profiles.
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  profiles::testing::CreateProfileSync(
      profile_manager, profile_manager->GenerateNextProfileDirectoryPath());
}

IN_PROC_BROWSER_TEST_F(BraveAvatarToolbarButtonBrowserTest,
                       NoSignInPromoWhenSigninAllowed) {
  Profile* profile = browser()->GetProfile();
  ASSERT_TRUE(profile->GetPrefs()->GetBoolean(prefs::kSigninAllowed));

  base::test::TestFuture<signin::ProfileMenuAvatarButtonPromoInfo> promo_info;
  signin::ComputeProfileMenuAvatarButtonPromoInfo(
      *profile, promo_info.GetCallback(),
      /*allow_batch_upload_promos=*/true);
  EXPECT_EQ(promo_info.Take(), signin::ProfileMenuAvatarButtonPromoInfo());

  AvatarToolbarButton* avatar_button = GetAvatarButton();
  ASSERT_TRUE(avatar_button);
  ASSERT_TRUE(avatar_button->GetVisible());
  EXPECT_NE(avatar_button->GetText(),
            l10n_util::GetStringUTF16(IDS_AVATAR_BUTTON_SIGNIN_PROMO));
}
