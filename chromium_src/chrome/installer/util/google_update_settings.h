// This file changes function signatures that used to be upstream and had to be
// restored in Brave to support delta updates on Windows until we are on Omaha
// 4. See github.com/brave/brave-core/pull/31937.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_UTIL_GOOGLE_UPDATE_SETTINGS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_UTIL_GOOGLE_UPDATE_SETTINGS_H_

// This method changes the Google Update "ap" value to move the installation
// on to or off of one of the recovery channels.
// - If incremental installer fails we append a magic string ("-full"), if
// it is not present already, so that Google Update server next time will send
// full installer to update Chrome on the local machine
// - If we are currently running full installer, we remove this magic
// string (if it is present) regardless of whether installer failed or not.
// There is no fall-back for full installer :)
// - Unconditionally clear a legacy "-stage:" modifier.
#define UpdateInstallStatus UpdateInstallStatus( \
  bool system_install, \
  installer::ArchiveType archive_type, \
  int install_return_code); \
  static void UpdateInstallStatus_Unused

// This method updates the value for Google Update "ap" key for Chrome
// based on whether we are doing incremental install (or not) and whether
// the install succeeded.
// - If install worked, remove the magic string (if present).
// - If incremental installer failed, append a magic string (if
//   not present already).
// - If full installer failed, still remove this magic
//   string (if it is present already).
// Additionally, any legacy ""-stage:*" values are
// unconditionally removed.
//
// archive_type: tells whether this is incremental install or not.
// install_return_code: if 0, means installation was successful.
// value: current value of Google Update "ap" key.
#define UpdateGoogleUpdateApKey UpdateGoogleUpdateApKey( \
  installer::ArchiveType archive_type, \
  int install_return_code, \
  installer::AdditionalParameters* value); \
  static bool UpdateGoogleUpdateApKey_Unused

#include <chrome/installer/util/google_update_settings.h>

#undef UpdateGoogleUpdateApKey
#undef UpdateInstallStatus

#endif  // BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_UTIL_GOOGLE_UPDATE_SETTINGS_H_
