/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"

#include <components/history_clusters/core/features.cc>

namespace history_clusters::internal {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kHistoryClustersInternalsPage, base::FEATURE_DISABLED_BY_DEFAULT},
    {kHistoryClustersNavigationContextClustering,
     base::FEATURE_DISABLED_BY_DEFAULT},
    {kJourneys, base::FEATURE_DISABLED_BY_DEFAULT},
    {kJourneysImages, base::FEATURE_DISABLED_BY_DEFAULT},
    {kOmniboxHistoryClusterProvider, base::FEATURE_DISABLED_BY_DEFAULT},
}});
}  // namespace history_clusters::internal
