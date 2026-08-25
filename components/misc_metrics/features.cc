/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/features.h"

#include "base/feature_list.h"
#include "base/time/time.h"

namespace misc_metrics::features {

BASE_FEATURE(kFingerprintInputMetrics, base::FEATURE_DISABLED_BY_DEFAULT);

const base::FeatureParam<base::TimeDelta> kFingerprintInputRendererInterval{
    &kFingerprintInputMetrics, "renderer_interval", base::Hours(10)};

}  // namespace misc_metrics::features
