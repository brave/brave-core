/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/profiles/brave_incognito_menu_view.h"

#include <memory>
#include <utility>

#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/views/profiles/brave_profile_menu_view_helper.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/hover_button.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/controls/button/button.h"

void BraveIncognitoMenuView::BuildMenu() {
  if (!brave::IsTorProfile(browser()->profile())) {
    IncognitoMenuView::BuildMenu();
    return;
  }

  ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();
    const SkColor icon_color = provider->GetTypographyProvider().GetColor(
        *this, views::style::CONTEXT_LABEL, views::style::STYLE_PRIMARY);
    auto incognito_icon = std::make_unique<views::ImageView>();
    incognito_icon->SetImage(
        gfx::CreateVectorIcon(kIncognitoProfileIcon, icon_color));

    AddMenuGroup(false /* add_separator */);
    CreateAndAddTitleCard(std::move(incognito_icon),
                          l10n_util::GetStringUTF16(IDS_TOR_PROFILE_NAME),
                          base::string16(), base::RepeatingClosure());

    AddMenuGroup();
    exit_button_ = CreateAndAddButton(
        gfx::CreateVectorIcon(kCloseAllIcon, 16, gfx::kChromeIconGrey),
        l10n_util::GetStringUTF16(IDS_PROFILES_EXIT_TOR),
        base::BindRepeating(&IncognitoMenuView::OnExitButtonClicked,
                            base::Unretained(this)));
    return;

    SetIdentityInfo(
        gfx::Image(gfx::CreateVectorIcon(kIncognitoProfileIcon, icon_color)),
        l10n_util::GetStringUTF16(IDS_TOR_PROFILE_NAME), base::string16());
}

void BraveIncognitoMenuView::AddTorButton() {
  if (brave::ShouldShowTorProfileButton(browser()->profile())) {
    tor_profile_button_ = CreateAndAddButton(
        brave::CreateTorProfileButtonIcon(),
        brave::CreateTorProfileButtonText(),
        base::BindRepeating(&BraveIncognitoMenuView::OnTorProfileButtonClicked,
                            base::Unretained(this)));
  }
}

void BraveIncognitoMenuView::OnTorProfileButtonClicked() {
  profiles::SwitchToTorProfile(ProfileManager::CreateCallback());
}
