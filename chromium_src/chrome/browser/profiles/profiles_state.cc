/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/brave_profile_manager.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "ui/gfx/text_elider.h"
#define SetActiveProfileToGuestIfLocked \
  SetActiveProfileToGuestIfLocked_ChromiumImpl
#define GetAvatarNameForProfile GetAvatarNameForProfile_ChromiumImpl
#include "../../../../../../chrome/browser/profiles/profiles_state.cc"
#undef GetAvatarNameForProfile
#undef SetActiveProfileToGuestIfLocked

namespace profiles {

#if !defined(OS_ANDROID)
base::string16 GetAvatarNameForProfile(const base::FilePath& profile_path) {
  if (profile_path == BraveProfileManager::GetTorProfilePath())
    return l10n_util::GetStringUTF16(IDS_TOR_PROFILE_NAME);
  return GetAvatarNameForProfile_ChromiumImpl(profile_path);
}
#if !defined(OS_CHROMEOS)
bool SetActiveProfileToGuestIfLocked() {
  ProfileManager* profile_manager = g_browser_process->profile_manager();

  const base::FilePath& active_profile_path =
      profile_manager->GetLastUsedProfileDir(profile_manager->user_data_dir());
  const base::FilePath& tor_path = BraveProfileManager::GetTorProfilePath();
  if (active_profile_path == tor_path)
    return true;

  return SetActiveProfileToGuestIfLocked_ChromiumImpl();
}
#endif  // !defined(OS_CHROMEOS)
#endif  // !defined(OS_ANDROID)

}  // namespace profiles
