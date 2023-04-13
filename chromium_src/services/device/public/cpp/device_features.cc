/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/services/device/public/cpp/device_features.cc"

#include "base/feature_override.h"
#include "build/build_config.h"

namespace features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
#if BUILDFLAG(IS_WIN)
    {kWinrtGeolocationImplementation, base::FEATURE_ENABLED_BY_DEFAULT},
#endif  // BUILDFLAG(IS_WIN)
}});

BASE_FEATURE(kLinuxGeoClueLocationBackend,
             "LinuxGeoClueLocationBackend",
             base::FEATURE_ENABLED_BY_DEFAULT);

}  // namespace features
