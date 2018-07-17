/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/version_info_values.h"

namespace version_info {
std::string GetBraveVersionNumber() {
  return std::string(BRAVE_BROWSER_VERSION) +
      "  Chromium: " + BRAVE_CHROMIUM_VERSION;
}
}  // namespace version_info
