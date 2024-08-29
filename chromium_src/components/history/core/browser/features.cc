/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/history/core/browser/features.cc"

#include "base/feature_override.h"

namespace history {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kOrganicRepeatableQueries, base::FEATURE_DISABLED_BY_DEFAULT},
}});

BASE_FEATURE(kHistoryMoreSearchResults,
             "HistoryMoreSearchResults",
             base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace history
