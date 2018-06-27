/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Brand-specific types and constants for Google Chrome.

#ifndef CHROME_INSTALL_STATIC_CHROMIUM_INSTALL_MODES_H_
#define CHROME_INSTALL_STATIC_CHROMIUM_INSTALL_MODES_H_

namespace install_static {

enum : bool {
#if defined(OFFICIAL_BUILD)
  kUseGoogleUpdateIntegration = true,
#else
  kUseGoogleUpdateIntegration = false,
#endif
};

// Note: This list of indices must be kept in sync with the brand-specific
// resource strings in chrome/installer/util/prebuild/create_string_rc.
enum InstallConstantIndex {
#if defined(OFFICIAL_BUILD)
  STABLE_INDEX,
  BETA_INDEX,
  DEV_INDEX,
  NIGHTLY_INDEX,
 #else
  DEVELOPER_INDEX,
 #endif
  NUM_INSTALL_MODES,
};

}  // namespace install_static

#endif  // CHROME_INSTALL_STATIC_CHROMIUM_INSTALL_MODES_H_
