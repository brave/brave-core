/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_MINI_INSTALLER_APPID_H_
#define BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_MINI_INSTALLER_APPID_H_

// The appid included by the mini_installer.
namespace google_update {

extern const wchar_t kAppGuid[];
extern const wchar_t kMultiInstallAppGuid[];

#if defined(OFFICIAL_BUILD)
extern const wchar_t kBetaAppGuid[];
extern const wchar_t kDevAppGuid[];
extern const wchar_t kSxSAppGuid[];
#endif  // defined(OFFICIAL_BUILD)

}  // namespace google_update

#endif  // BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_MINI_INSTALLER_APPID_H_
