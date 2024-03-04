/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/password_manager/core/common/password_manager_features.cc"

#include "base/feature_override.h"
#include "build/build_config.h"

namespace password_manager::features {

#if BUILDFLAG(IS_ANDROID)
OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kUnifiedPasswordManagerLocalPasswordsMigrationWarning,
     base::FEATURE_DISABLED_BY_DEFAULT},
}});
#endif

}  // namespace password_manager::features
