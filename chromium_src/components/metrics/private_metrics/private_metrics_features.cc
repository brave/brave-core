/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"

#include <components/metrics/private_metrics/private_metrics_features.cc>

namespace metrics::private_metrics {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kPrivateMetricsFeature, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace metrics::private_metrics
