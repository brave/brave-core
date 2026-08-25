/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_FEATURES_H_
#define BRAVE_COMPONENTS_MISC_METRICS_FEATURES_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"

namespace misc_metrics::features {

// Enables periodic collection and reporting of browser fingerprint input
// stability metrics.
BASE_DECLARE_FEATURE(kFingerprintInputMetrics);

// The interval between executions of the fingerprint stability script in the
// renderer.
extern const base::FeatureParam<base::TimeDelta>
    kFingerprintInputRendererInterval;

}  // namespace misc_metrics::features

#endif  // BRAVE_COMPONENTS_MISC_METRICS_FEATURES_H_
