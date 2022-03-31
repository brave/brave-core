/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_FREQUENCY_CAPPING_ANTI_TARGETING_ANTI_TARGETING_FEATURES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_FREQUENCY_CAPPING_ANTI_TARGETING_ANTI_TARGETING_FEATURES_H_

#include "base/feature_list.h"

namespace ads {
namespace resource {
namespace features {

extern const base::Feature kAntiTargeting;

bool IsAntiTargetingEnabled();

int GetAntiTargetingResourceVersion();

}  // namespace features
}  // namespace resource
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_FREQUENCY_CAPPING_ANTI_TARGETING_ANTI_TARGETING_FEATURES_H_
