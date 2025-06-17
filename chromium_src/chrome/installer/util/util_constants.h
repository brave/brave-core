/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_UTIL_UTIL_CONSTANTS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_UTIL_UTIL_CONSTANTS_H_

#define kGoogleUpdateIsMachineEnvVar                               \
  kGoogleUpdateIsMachineEnvVar[] = "BraveSoftwareUpdateIsMachine"; \
  inline constexpr char kGoogleUpdateIsMachineEnvVar_UnUsed

#include <chrome/installer/util/util_constants.h>  // IWYU pragma: export
#undef kGoogleUpdateIsMachineEnvVar

#endif  // BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_UTIL_UTIL_CONSTANTS_H_
