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
    {kExpandedStateHeight, base::FEATURE_DISABLED_BY_DEFAULT},
    {kExpandedStateShape, base::FEATURE_DISABLED_BY_DEFAULT},
    {kOmniboxSteadyStateHeight, base::FEATURE_DISABLED_BY_DEFAULT},
    {kRichAutocompletion, base::FEATURE_DISABLED_BY_DEFAULT},
}});

BASE_FEATURE(kOmniboxTabSwitchByDefault,
             "OmniboxTabSwitchByDefault",
             base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace omnibox
