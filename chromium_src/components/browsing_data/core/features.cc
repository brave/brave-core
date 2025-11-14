/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"
#include "build/build_config.h"

#include <components/browsing_data/core/features.cc>

namespace browsing_data::features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
    {kDbdRevampDesktop, base::FEATURE_DISABLED_BY_DEFAULT},
#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
}});

}  // namespace browsing_data::features
