/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/password_manager/core/browser/features/password_features.cc"

#include "base/feature_override.h"
#include "build/build_config.h"

namespace password_manager::features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
#if BUILDFLAG(IS_IOS) || BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC) || \
    BUILDFLAG(IS_WIN)
    {kSkipUndecryptablePasswords, base::FEATURE_ENABLED_BY_DEFAULT},
#endif
#if BUILDFLAG(IS_ANDROID)
    // This flag will be removed eventually, so we should move LoginDb related
    // code to brave-core if we want to keep it. The follow up issue is created
    // for this: https://github.com/brave/brave-browser/issues/47419
    {kLoginDbDeprecationAndroid, base::FEATURE_DISABLED_BY_DEFAULT},
#endif  // BUILDFLAG(IS_ANDROID)
}});

}  // namespace password_manager::features
