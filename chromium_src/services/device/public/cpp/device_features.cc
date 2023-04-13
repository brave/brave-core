/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "services/device/public/cpp/device_features.h"

#define kLocationProviderManagerParam kLocationProviderManagerParamUnused
#include "src/services/device/public/cpp/device_features.cc"
#undef kLocationProviderManagerParam

#include "base/feature_override.h"

namespace features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kLocationProviderManager, base::FEATURE_ENABLED_BY_DEFAULT},
}});

const base::FeatureParam<device::mojom::LocationProviderManagerMode>
    kLocationProviderManagerParam{
        &kLocationProviderManager, "LocationProviderManagerMode",
        device::mojom::LocationProviderManagerMode::kPlatformOnly,
        &location_provider_manager_mode_options};

BASE_FEATURE(kLinuxGeoClueLocationBackend,
             "LinuxGeoClueLocationBackend",
             base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace features
