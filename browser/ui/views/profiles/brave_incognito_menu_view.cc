/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/profiles/brave_incognito_menu_view.h"

#include <memory>
#include <utility>

#include "brave/browser/ui/views/profiles/brave_profile_menu_view_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/hover_button.h"
#include "ui/views/controls/button/button.h"

void BraveIncognitoMenuView::ButtonPressed(views::Button* sender,
                                           const ui::Event& event) {
  if (sender == tor_profile_button_) {
    profiles::SwitchToTorProfile(ProfileManager::CreateCallback());
  } else {
    IncognitoMenuView::ButtonPressed(sender, event);
  }
}

void BraveIncognitoMenuView::AddTorButton() {
  if (brave::ShouldShowTorProfileButton(browser()->profile())) {
    tor_profile_button_ = CreateAndAddButton(
        brave::CreateTorProfileButtonIcon(),
        brave::CreateTorProfileButtonText());
  }
}

void BraveIncognitoMenuView::Reset() {
  IncognitoMenuView::Reset();
  tor_profile_button_ = nullptr;
}
