/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/version_info/version_info.h"

namespace version_info {

std::string GetBraveVersionWithoutChromiumMajorVersion() {
  return std::string(BRAVE_BROWSER_VERSION);
}

std::string GetBraveVersionNumberForDisplay() {
  return std::string(BRAVE_BROWSER_VERSION) +
         "  Chromium: " + BRAVE_CHROMIUM_VERSION;
}

std::string GetBraveChromiumVersionNumber() {
  return std::string(BRAVE_CHROMIUM_VERSION);
}

}  // namespace version_info
