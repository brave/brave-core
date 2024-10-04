/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/installer/util/util_constants.h"

#if defined(OFFICIAL_BUILD)
#define kGoogleUpdateIsMachineEnvVar kGoogleUpdateIsMachineEnvVar_ChromiumImpl
#endif

#define kChromeExe kChromeExe_Unused

#include "src/chrome/installer/util/util_constants.cc"

#if defined(OFFICIAL_BUILD)
#undef kGoogleUpdateIsMachineEnvVar
#endif

#undef kChromeExe

namespace installer {

namespace env_vars {

#if defined(OFFICIAL_BUILD)
constexpr char kGoogleUpdateIsMachineEnvVar[] = "BraveSoftwareUpdateIsMachine";
#endif

}  // namespace env_vars

const wchar_t kChromeExe[] = L"brave.exe";

}  // namespace installer
