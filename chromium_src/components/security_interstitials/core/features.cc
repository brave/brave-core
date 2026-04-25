/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"
#include "build/build_config.h"

#include <components/security_interstitials/core/features.cc>

namespace security_interstitials::features {

#if BUILDFLAG(IS_ANDROID)
OVERRIDE_FEATURE_DEFAULT_STATES({{
    // Disable dialog UI on Android since Android doesn't have the dialog
    // implementation and should use the full-page interstitial instead.
    {kHttpsFirstDialogUi, base::FEATURE_DISABLED_BY_DEFAULT},
}});
#endif

}  // namespace security_interstitials::features
