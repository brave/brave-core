/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_FLAGS_FLAG_MANAGER_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_FLAGS_FLAG_MANAGER_CONSTANTS_H_

#include "brave/components/brave_ads/core/internal/flags/environment/environment_types.h"
#include "build/build_config.h"

namespace ads {

#if defined(OFFICIAL_BUILD)
constexpr EnvironmentType kDefaultEnvironmentType =
    EnvironmentType::kProduction;
#else   // OFFICIAL_BUILD
constexpr EnvironmentType kDefaultEnvironmentType = EnvironmentType::kStaging;
#endif  // !OFFICIAL_BUILD

}  // namespace ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_FLAGS_FLAG_MANAGER_CONSTANTS_H_
