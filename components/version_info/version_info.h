/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_VERSION_INFO_VERSION_INFO_H_
#define BRAVE_COMPONENTS_VERSION_INFO_VERSION_INFO_H_

#include <string>

#include "brave/components/version_info/version_info_values.h"

namespace version_info {

std::string GetBraveVersionWithoutChromiumMajorVersion();

constexpr std::string GetBraveVersionNumberForDisplay() {
  return constants::kBraveVersionNumberForDisplay;
}

std::string GetBraveChromiumVersionNumber();

}  // namespace version_info

#endif  // BRAVE_COMPONENTS_VERSION_INFO_VERSION_INFO_H_
