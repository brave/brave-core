/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/installer/util/util_constants.h"

#define kGoogleUpdateIsMachineEnvVar kGoogleUpdateIsMachineEnvVar_ChromiumImpl
#define kChromeExe kChromeExe_Unused

#include "../../../../../chrome/installer/util/util_constants.cc"

#undef kGoogleUpdateIsMachineEnvVar
#undef kChromeExe

namespace installer {

namespace env_vars {

#if defined(OFFICIAL_BUILD)
const char kGoogleUpdateIsMachineEnvVar[] = "BraveSoftwareUpdateIsMachine";
#else
const char kGoogleUpdateIsMachineEnvVar[] =
    kGoogleUpdateIsMachineEnvVar_ChromiumImpl;
#endif

}  // namespace env_vars

const wchar_t kChromeExe[] = L"brave.exe";

}  // namespace installer
