/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_ANTI_TARGETING_ANTI_TARGETING_INFO_ALIASES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_ANTI_TARGETING_ANTI_TARGETING_INFO_ALIASES_H_

#include <map>
#include <set>
#include <string>

#include "url/gurl.h"

namespace ads {
namespace resource {

using AntiTargetingSiteList = std::set<GURL>;
using AntiTargetingMap = std::map<std::string, AntiTargetingSiteList>;

}  // namespace resource
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_ANTI_TARGETING_ANTI_TARGETING_INFO_ALIASES_H_
