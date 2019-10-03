/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_VERSION_INFO_H_
#define BRAVE_BROWSER_VERSION_INFO_H_

#include <string>

namespace version_info {
std::string GetBraveVersionWithoutChromiumMajorVersion();
std::string GetBraveVersionNumberForDisplay();
}  // namespace version_info

#endif  // BRAVE_BROWSER_VERSION_INFO_H_
