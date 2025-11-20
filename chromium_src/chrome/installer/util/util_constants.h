/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// The macros INSTALL_FAILED, NO_STAGE, UNCOMPRESSING and the enum ArchiveType
// below contain code that used to be upstream and had to be restored in Brave
// to support delta updates on Windows until we are on Omaha 4. See:
// github.com/brave/brave-core/pull/31937

#ifndef BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_UTIL_UTIL_CONSTANTS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_UTIL_UTIL_CONSTANTS_H_

#define INSTALL_FAILED \
  SETUP_PATCH_FAILED = 8, \
  APPLY_DIFF_PATCH_FAILED = 42, \
  PATCH_INVALID_ARGUMENTS = 49, \
  DIFF_PATCH_SOURCE_MISSING = 50, \
  INSTALL_FAILED

#define NO_STAGE \
  NO_STAGE, \
  UPDATING_SETUP // Patching setup.exe with differential update.

#define UNCOMPRESSING \
  UNCOMPRESSING, \
  PATCHING // Patching chrome.7z with differential update.

#define kGoogleUpdateIsMachineEnvVar                               \
  kGoogleUpdateIsMachineEnvVar[] = "BraveSoftwareUpdateIsMachine"; \
  inline constexpr char kGoogleUpdateIsMachineEnvVar_UnUsed

#include <chrome/installer/util/util_constants.h>  // IWYU pragma: export

#undef UNCOMPRESSING
#undef NO_STAGE
#undef INSTALL_FAILED
#undef kGoogleUpdateIsMachineEnvVar

namespace installer {

// The type of an update archive.
enum ArchiveType {
  UNKNOWN_ARCHIVE_TYPE,     // Unknown or uninitialized.
  FULL_ARCHIVE_TYPE,        // Full chrome.7z archive.
  INCREMENTAL_ARCHIVE_TYPE  // Incremental or differential archive.
};

}  // namespace installer

#endif  // BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_UTIL_UTIL_CONSTANTS_H_
