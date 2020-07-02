/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/browser/profiles/profile_util.h"
#include "ui/gfx/text_elider.h"
#define GetAvatarNameForProfile GetAvatarNameForProfile_ChromiumImpl
#include "../../../../../chrome/browser/profiles/profiles_state.cc"
#undef GetAvatarNameForProfile

namespace profiles {

#if !defined(OS_ANDROID)
base::string16 GetAvatarNameForProfile(const base::FilePath& profile_path) {
  if (brave::IsTorProfilePath(profile_path))
    return l10n_util::GetStringUTF16(IDS_TOR_PROFILE_NAME);
  return GetAvatarNameForProfile_ChromiumImpl(profile_path);
}
#endif  // !defined(OS_ANDROID)

}  // namespace profiles
