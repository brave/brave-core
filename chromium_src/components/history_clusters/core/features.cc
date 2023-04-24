/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/history_clusters/core/features.cc"

#include "base/feature_override.h"

namespace history_clusters {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {internal::kHideVisits, base::FEATURE_DISABLED_BY_DEFAULT},
    {internal::kHistoryClustersInternalsPage,
     base::FEATURE_DISABLED_BY_DEFAULT},
    {internal::kOmniboxHistoryClusterProvider,
     base::FEATURE_DISABLED_BY_DEFAULT},
    {internal::kJourneys, base::FEATURE_DISABLED_BY_DEFAULT},
    {internal::kOmniboxAction, base::FEATURE_DISABLED_BY_DEFAULT},
    {internal::kPersistedClusters, base::FEATURE_DISABLED_BY_DEFAULT},
    {internal::kPersistContextAnnotationsInHistoryDb,
     base::FEATURE_DISABLED_BY_DEFAULT},
    {kSidePanelJourneys, base::FEATURE_DISABLED_BY_DEFAULT},
}});
}  // namespace history_clusters
