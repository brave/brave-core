/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FLAGS_FLAG_MANAGER_CONSTANTS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FLAGS_FLAG_MANAGER_CONSTANTS_H_

#include "bat/ads/internal/flags/environment/environment_types.h"
#include "build/build_config.h"  // IWYU pragma: keep

namespace ads {

#if defined(OFFICIAL_BUILD)
constexpr EnvironmentType kDefaultEnvironmentType =
    EnvironmentType::kProduction;
#else   // OFFICIAL_BUILD
constexpr EnvironmentType kDefaultEnvironmentType = EnvironmentType::kStaging;
#endif  // !OFFICIAL_BUILD

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FLAGS_FLAG_MANAGER_CONSTANTS_H_
