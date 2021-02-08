/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/brave_profile_shortcut_manager_win.h"

#include "brave/browser/profiles/profile_util.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"

BraveProfileShortcutManagerWin::BraveProfileShortcutManagerWin(
    ProfileManager* manager)
    : ProfileShortcutManagerWin(manager),
      profile_manager_(manager) {}

void BraveProfileShortcutManagerWin::GetShortcutProperties(
    const base::FilePath& profile_path,
    base::CommandLine* command_line,
    base::string16* name,
    base::FilePath* icon_path) {
  // Session profiles are currently not added into storage because they will
  // return early in ProfileManager::AddProfileToStorage because of the profile
  // path is not directly under user_data_dir. To avoid DCHECK(has_entry) in
  // chromium's GetShortcutProperties and invalid access to the entry, return
  // early here when entry cannot be found.
  //
  // TODO(jocelyn): Properly add session profiles into the storage and remove
  // this override.
  if (brave::IsSessionProfilePath(profile_path)) {
    ProfileAttributesStorage& storage =
        profile_manager_->GetProfileAttributesStorage();
    if (!storage.GetProfileAttributesWithPath(profile_path))
      return;
  }

  ProfileShortcutManagerWin::GetShortcutProperties(profile_path,
                                                   command_line,
                                                   name,
                                                   icon_path);
}
