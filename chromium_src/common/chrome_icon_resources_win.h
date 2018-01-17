/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_BRAVE_ICON_RESOURCES_WIN_H_
#define BRAVE_COMMON_BRAVE_ICON_RESOURCES_WIN_H_

namespace icon_resources {

// This file contains the indices of icon resources in chrome_exe.rc.

enum {
  // The main application icon is always index 0.
  kApplicationIndex = 0,

  // Legacy indices that are no longer used.
  kApplication2Index = 1,
  kApplication3Index = 2,
  kApplication4Index = 3,

  // The Chrome Canary application icon.
  kSxSApplicationIndex = 4,

  // The Chrome App Launcher icon.
  kAppLauncherIndex = 5,

  // The Chrome App Launcher Canary icon.
  kSxSAppLauncherIndex = 6,

  // The Chrome incognito icon.
  kIncognitoIndex = 7,

  // The Chrome Dev application icon.
  kDevApplicationIndex = 8,

  // The Chrome Beta application icon.
  kBetaApplicationIndex = 9,
};

}  // namespace icon_resources

#endif  // BRAVE_COMMON_BRAVE_ICON_RESOURCES_WIN_H_
