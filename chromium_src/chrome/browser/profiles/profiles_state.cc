/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/browser/profiles/profile_util.h"
#include "ui/gfx/text_elider.h"
#define GetAvatarButtonTextForProfile GetAvatarButtonTextForProfile_ChromiumImpl
#define GetAvatarNameForProfile GetAvatarNameForProfile_ChromiumImpl
#include "../../../../../../chrome/browser/profiles/profiles_state.cc"
#undef GetAvatarNameForProfile
#undef GetAvatarButtonTextForProfile

namespace profiles {

#if !defined(OS_ANDROID)
base::string16 GetAvatarNameForProfile(const base::FilePath& profile_path) {
  if (brave::IsTorProfilePath(profile_path))
    return l10n_util::GetStringUTF16(IDS_TOR_PROFILE_NAME);
  return GetAvatarNameForProfile_ChromiumImpl(profile_path);
}

#if !defined(OS_CHROMEOS)
base::string16 GetAvatarButtonTextForProfile(Profile* profile) {
  const int kMaxCharactersToDisplay = 15;
  base::string16 name = GetAvatarNameForProfile(profile->GetPath());
  name = gfx::TruncateString(name,
                             kMaxCharactersToDisplay,
                             gfx::CHARACTER_BREAK);
  if (profile->IsLegacySupervised()) {
    name = l10n_util::GetStringFUTF16(
        IDS_LEGACY_SUPERVISED_USER_NEW_AVATAR_LABEL, name);
  }
  return name;
}
#endif  // !defined(OS_CHROMEOS)
#endif  // !defined(OS_ANDROID)

}  // namespace profiles
