/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/profiles/brave_profile_chooser_view.h"

#include <memory>
#include <utility>

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/extensions/brave_tor_client_updater.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/views/hover_button.h"
#include "chrome/browser/ui/views/profiles/badged_profile_photo.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/paint_vector_icon.h"

void BraveProfileChooserView::AddDiceSyncErrorView(
    const AvatarMenu::Item& avatar_item,
    sync_ui_util::AvatarSyncErrorType error,
    int button_string_id) {
  ProfileMenuViewBase::MenuItems menu_items;

  auto current_profile_photo = std::make_unique<BadgedProfilePhoto>(
      BadgedProfilePhoto::BADGE_TYPE_NONE, avatar_item.icon);
  base::string16 profile_name = avatar_item.name;
  if (profile_name.empty()) {
    Profile* profile = browser()->profile();
    profile_name = profiles::GetAvatarNameForProfile(profile->GetPath());
  }

  std::unique_ptr<HoverButton> current_profile = std::make_unique<HoverButton>(
      this, std::move(current_profile_photo), profile_name, base::string16());
  current_profile->SetAccessibleName(l10n_util::GetStringFUTF16(
      IDS_PROFILES_EDIT_PROFILE_ACCESSIBLE_NAME, profile_name));

  current_profile_card_ = current_profile.get();
  menu_items.push_back(std::move(current_profile));
  AddMenuItems(menu_items, true);
}
