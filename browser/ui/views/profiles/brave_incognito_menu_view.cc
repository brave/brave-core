/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/profiles/brave_incognito_menu_view.h"

#include <memory>
#include <utility>

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/tor/buildflags.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/grit/generated_resources.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/l10n/l10n_util.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/extensions/brave_tor_client_updater.h"
#include "brave/browser/tor/tor_profile_service.h"
#endif

namespace {

bool ShouldShowTorProfileButton(Profile* profile) {
  DCHECK(profile);
#if BUILDFLAG(ENABLE_TOR)
  return !tor::TorProfileService::IsTorDisabled() &&
         !brave::IsTorProfile(profile) &&
         !g_brave_browser_process->tor_client_updater()
              ->GetExecutablePath()
              .empty();
#else
  return false;
#endif
}

int GetProfileMenuTitleId(Profile* profile) {
  return brave::IsTorProfile(profile) ? IDS_TOR_PROFILE_NAME
                                      : IDS_INCOGNITO_PROFILE_MENU_TITLE;
}

int GetProfileMenuCloseButtonTextId(Profile* profile) {
  return brave::IsTorProfile(profile) ? IDS_PROFILES_EXIT_TOR
                                      : IDS_INCOGNITO_PROFILE_MENU_CLOSE_BUTTON;
}

int GetWindowCount(Profile* profile) {
  return brave::IsTorProfile(profile)
             ? 0
             : BrowserList::GetOffTheRecordBrowsersActiveForProfile(profile);
}

}  // namespace

void BraveIncognitoMenuView::BuildMenu() {
  ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();
  // The icon color is set to match the menu text, which guarantees sufficient
  // contrast and a consistent visual appearance.
  const SkColor icon_color = provider->GetTypographyProvider().GetColor(
      *this, views::style::CONTEXT_LABEL, views::style::STYLE_PRIMARY);

  int window_count = GetWindowCount(browser()->profile());
  SetProfileIdentityInfo(/*profile_name=*/base::string16(),
                         /*edit_button=*/base::nullopt,
                         ColoredImageForMenu(kIncognitoProfileIcon, icon_color),
                         l10n_util::GetStringUTF16(
                           GetProfileMenuTitleId(browser()->profile())),
                         window_count > 1 ? l10n_util::GetPluralStringFUTF16(
                             IDS_INCOGNITO_WINDOW_COUNT_MESSAGE, window_count)
                           : base::string16());

  AddTorButton();

  AddFeatureButton(l10n_util::GetStringUTF16(
                       GetProfileMenuCloseButtonTextId(browser()->profile())),
                   base::BindRepeating(&IncognitoMenuView::OnExitButtonClicked,
                                       base::Unretained(this)),
                   kCloseAllIcon);
}

void BraveIncognitoMenuView::AddTorButton() {
  if (ShouldShowTorProfileButton(browser()->profile())) {
    AddFeatureButton(
        l10n_util::GetStringUTF16(IDS_PROFILES_OPEN_TOR_PROFILE_BUTTON),
        base::BindRepeating(&BraveIncognitoMenuView::OnTorProfileButtonClicked,
                            base::Unretained(this)),
        vector_icons::kLaunchIcon);
  }
}

void BraveIncognitoMenuView::OnTorProfileButtonClicked() {
  profiles::SwitchToTorProfile(ProfileManager::CreateCallback());
}

base::string16 BraveIncognitoMenuView::GetAccessibleWindowTitle() const {
  return brave::IsTorProfile(browser()->profile())
             ? l10n_util::GetStringUTF16(IDS_TOR_PROFILE_NAME)
             : IncognitoMenuView::GetAccessibleWindowTitle();
}

void BraveIncognitoMenuView::OnExitButtonClicked() {
  if (brave::IsTorProfile(browser()->profile())) {
    RecordClick(ActionableItem::kExitProfileButton);
    profiles::CloseTorProfileWindows();
  } else {
    IncognitoMenuView::OnExitButtonClicked();
  }
}
