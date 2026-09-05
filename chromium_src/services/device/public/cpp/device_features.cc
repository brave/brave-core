/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "services/device/public/cpp/device_features.h"

#define kLocationProviderManagerParam kLocationProviderManagerParamUnused
#include <services/device/public/cpp/device_features.cc>
#undef kLocationProviderManagerParam

namespace features {

const base::FeatureParam<device::mojom::LocationProviderManagerMode>
    kLocationProviderManagerParam{
        &kLocationProviderManager, "LocationProviderManagerMode",
        device::mojom::LocationProviderManagerMode::kPlatformOnly,
        &location_provider_manager_mode_options};

}  // namespace features
