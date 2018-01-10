/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Brand-specific types and constants for Google Chrome.

#ifndef BRAVE_INSTALL_STATIC_BRAVE_INSTALL_MODES_H_
#define BRAVE_INSTALL_STATIC_BRAVE_INSTALL_MODES_H_

namespace install_static {

enum : bool {
  kUseGoogleUpdateIntegration = true,
};

// Note: This list of indices must be kept in sync with the brand-specific
// resource strings in chrome/installer/util/prebuild/create_string_rc.
enum InstallConstantIndex {
  STABLE_INDEX,
  BETA_INDEX,
  DEV_INDEX,
  CANARY_INDEX,
  NUM_INSTALL_MODES,
};

}  // namespace install_static

#endif  // BRAVE_INSTALL_STATIC_BRAVE_INSTALL_MODES_H_
