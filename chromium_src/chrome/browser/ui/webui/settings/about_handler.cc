/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include "brave/version.h"
#include "components/version_info/version_info_values.h"

namespace version_info {
std::string GetBraveVersionNumber() {
  return std::string(BRAVE_BROWSER_VERSION) + "  Chromium: " + PRODUCT_VERSION;
}
}  // namespace version_info

#define GetVersionNumber GetBraveVersionNumber
#include "../../../../../../../chrome/browser/ui/webui/settings/about_handler.cc"
#undef GetVersionNumber
