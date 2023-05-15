/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/segmentation_platform/public/features.h"

#include "src/components/segmentation_platform/public/features.cc"

#include "base/feature_override.h"

namespace segmentation_platform::features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kSegmentationPlatformDeviceTier, base::FEATURE_DISABLED_BY_DEFAULT},
    {kSegmentationPlatformFeature, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace segmentation_platform::features
