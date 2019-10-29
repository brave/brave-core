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
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/grit/generated_resources.h"
#include "ui/views/controls/button/button.h"

int BraveIncognitoMenuView::GetProfileMenuTitleId()
{
  return brave::IsTorProfile(browser()->profile())
             ? IDS_TOR_PROFILE_NAME
             : IDS_INCOGNITO_PROFILE_MENU_TITLE;
}

int BraveIncognitoMenuView::GetProfileMenuCloseButtonTextId()
{
  return brave::IsTorProfile(browser()->profile())
             ? IDS_PROFILES_EXIT_TOR
             : IDS_INCOGNITO_PROFILE_MENU_CLOSE_BUTTON;
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
