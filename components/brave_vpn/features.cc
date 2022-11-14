/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/features.h"

#include "base/feature_list.h"
#include "build/build_config.h"

namespace brave_vpn {

namespace features {

BASE_FEATURE(kBraveVPN,
             "BraveVPN",
#if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_IOS)
             base::FEATURE_ENABLED_BY_DEFAULT
#else
             base::FEATURE_DISABLED_BY_DEFAULT
#endif
);

}  // namespace features

}  // namespace brave_vpn
