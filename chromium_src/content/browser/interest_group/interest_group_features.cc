/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/content/browser/interest_group/interest_group_features.cc"

#include "base/feature_override.h"

namespace features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kEnableUpdatingExecutionModeToFrozenContext,
     base::FEATURE_DISABLED_BY_DEFAULT},
    {kEnableUpdatingUserBiddingSignals, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace features
