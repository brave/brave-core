/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/profiles/brave_profile_menu_view.h"

#include <memory>
#include <utility>

#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/views/profiles/brave_profile_menu_view_helper.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/views/hover_button.h"
#include "chrome/browser/ui/views/profiles/badged_profile_photo.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

void BraveProfileMenuView::AddAutofillHomeView() { }

void BraveProfileMenuView::AddDiceSyncErrorView(
    const AvatarMenu::Item& avatar_item,
    sync_ui_util::AvatarSyncErrorType error,
    int button_string_id) {
  auto current_profile_photo = std::make_unique<BadgedProfilePhoto>(
      BadgedProfilePhoto::BADGE_TYPE_NONE, avatar_item.icon);
  base::string16 profile_name = avatar_item.name;
  if (profile_name.empty()) {
    Profile* profile = browser()->profile();
    profile_name = profiles::GetAvatarNameForProfile(profile->GetPath());
  }

  AddMenuGroup();
  current_profile_card_ = CreateAndAddTitleCard(
      std::move(current_profile_photo), profile_name, base::string16(),
      base::BindRepeating(&BraveProfileMenuView::OnCurrentProfileCardClicked,
                          base::Unretained(this)));

  current_profile_card_->SetAccessibleName(l10n_util::GetStringFUTF16(
      IDS_PROFILES_EDIT_PROFILE_ACCESSIBLE_NAME, profile_name));
}

void BraveProfileMenuView::OnExitProfileButtonClicked() {
  if (browser()->profile()->IsGuestSession()) {
    RecordClick(ActionableItem::kExitProfileButton);
    if (brave::IsTorProfile(browser()->profile()))
      profiles::CloseTorProfileWindows();
    else
      profiles::CloseGuestProfileWindows();
  } else {
    ProfileMenuView::OnExitProfileButtonClicked();
  }
}
