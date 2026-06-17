/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"

#include <components/metrics/private_metrics/private_insights/private_insights_features.cc>

namespace private_insights {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kPrivateInsightsFeature, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace private_insights
