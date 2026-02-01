/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/serp_metrics_mock.h"

#include "components/prefs/pref_service.h"

namespace serp_metrics {

SerpMetricsMock::SerpMetricsMock(PrefService* local_state, PrefService* prefs)
    : SerpMetrics(local_state, prefs) {}

SerpMetricsMock::~SerpMetricsMock() = default;

}  // namespace serp_metrics
