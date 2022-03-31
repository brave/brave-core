/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/profiles/brave_incognito_menu_view.h"

#include <memory>
#include <utility>

#include "brave/components/tor/buildflags/buildflags.h"
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
#include "ui/base/models/image_model.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#endif

namespace {

bool ShouldShowTorProfileButton(Profile* profile) {
  DCHECK(profile);
#if BUILDFLAG(ENABLE_TOR)
  return !TorProfileServiceFactory::IsTorDisabled() &&
         !profile->IsTor();
#else
  return false;
#endif
}

int GetProfileMenuTitleId(Profile* profile) {
  return profile->IsTor() ? IDS_TOR_PROFILE_NAME : IDS_PRIVATE_PROFILE_NAME;
}

int GetProfileMenuCloseButtonTextId(Profile* profile) {
  return profile->IsTor() ? IDS_PROFILES_EXIT_TOR : IDS_PROFILES_EXIT_PRIVATE;
}

}  // namespace

void BraveIncognitoMenuView::BuildMenu() {
  AddTorButton();

  AddFeatureButton(
      l10n_util::GetStringUTF16(
          GetProfileMenuCloseButtonTextId(browser()->profile())),
      base::BindRepeating(&BraveIncognitoMenuView::OnExitButtonClicked,
                          base::Unretained(this)),
      vector_icons::kCloseIcon);
}

void BraveIncognitoMenuView::AddedToWidget() {
  IncognitoMenuView::AddedToWidget();

  ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();
  // The icon color is set to match the menu text, which guarantees sufficient
  // contrast and a consistent visual appearance.
  const SkColor icon_color = provider->GetTypographyProvider().GetColor(
      *this, views::style::CONTEXT_LABEL, views::style::STYLE_PRIMARY);

  int window_count = BrowserList::GetOffTheRecordBrowsersActiveForProfile(
      browser()->profile());
  SetProfileIdentityInfo(
      /*profile_name=*/std::u16string(),
      /*background_color=*/SK_ColorTRANSPARENT,
      /*edit_button=*/absl::nullopt,
      ui::ImageModel::FromVectorIcon(kIncognitoProfileIcon, icon_color),
      l10n_util::GetStringUTF16(GetProfileMenuTitleId(browser()->profile())),
      window_count > 1 ? l10n_util::GetPluralStringFUTF16(
                             IDS_INCOGNITO_WINDOW_COUNT_MESSAGE, window_count)
                       : std::u16string());
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
  TorProfileManager::SwitchToTorProfile(browser()->profile(),
                                        base::DoNothing());
}

std::u16string BraveIncognitoMenuView::GetAccessibleWindowTitle() const {
  return browser()->profile()->IsTor()
             ? l10n_util::GetStringUTF16(IDS_TOR_PROFILE_NAME)
             : IncognitoMenuView::GetAccessibleWindowTitle();
}

void BraveIncognitoMenuView::OnExitButtonClicked() {
  if (browser()->profile()->IsTor()) {
    RecordClick(ActionableItem::kExitProfileButton);
    TorProfileManager::CloseTorProfileWindows(browser()->profile());
  } else {
    IncognitoMenuView::OnExitButtonClicked();
  }
}
