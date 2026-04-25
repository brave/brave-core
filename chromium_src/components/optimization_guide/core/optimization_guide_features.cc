/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"

#include <components/optimization_guide/core/optimization_guide_features.cc>

namespace optimization_guide::features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kOptimizationGuideFetchingForSRP, base::FEATURE_DISABLED_BY_DEFAULT},
    {kOptimizationHints, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace optimization_guide::features
