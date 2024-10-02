/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"

#include "src/components/omnibox/common/omnibox_features.cc"

namespace omnibox {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kDocumentProviderNoSetting, base::FEATURE_DISABLED_BY_DEFAULT},
    {kDocumentProviderNoSyncRequirement, base::FEATURE_DISABLED_BY_DEFAULT},
    {kMlUrlScoring, base::FEATURE_DISABLED_BY_DEFAULT},
#if BUILDFLAG(IS_ANDROID)
    {kRetainOmniboxOnFocus, base::FEATURE_ENABLED_BY_DEFAULT},
#endif
    {kRichAutocompletion, base::FEATURE_DISABLED_BY_DEFAULT},
    {kStarterPackExpansion, base::FEATURE_DISABLED_BY_DEFAULT},
    {kZeroSuggestPrefetching, base::FEATURE_DISABLED_BY_DEFAULT},
}});

BASE_FEATURE(kOmniboxTabSwitchByDefault,
             "OmniboxTabSwitchByDefault",
             base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace omnibox
