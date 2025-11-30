/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_MINI_INSTALLER_MINI_INSTALLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_MINI_INSTALLER_MINI_INSTALLER_H_

#include <chrome/installer/mini_installer/mini_installer.h>  // IWYU pragma: export

namespace mini_installer {
typedef StackString<128> ReferralCodeString;

// Populates |referral_code| with a Brave referral code if one is
// present in the installer filename. This may be a standard or an
// extended referral code.

bool ParseReferralCode(const wchar_t* installer_filename,
                       ReferralCodeString* referral_code);

// Upstream's UnpackBinaryResources function used to have a parameter
// ResourceTypeString& setup_type. When this had value kLZMAResourceType after
// the function, then upstream used to run the previous setup.exe to patch and
// generate the new setup.exe. Upstream removed this functionality when it
// switched to Omaha 4. We are still on Omaha 3. To restore the feature with
// minimal changes in upstream even though UnpackBinaryResources's has changed,
// we stuff the information whether a differential update is to be applied into
// the Windows error code, with this special value:
inline constexpr DWORD kNotAnErrorIsPatchUpdate = 0x12345678;

}  // namespace mini_installer

#endif  // BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_MINI_INSTALLER_MINI_INSTALLER_H_
