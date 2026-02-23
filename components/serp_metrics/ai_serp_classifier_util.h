/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_AI_SERP_CLASSIFIER_UTIL_H_
#define BRAVE_COMPONENTS_SERP_METRICS_AI_SERP_CLASSIFIER_UTIL_H_

#include <optional>

#include "brave/components/serp_metrics/serp_metric_type.h"

class GURL;

namespace serp_metrics {

std::optional<SerpMetricType> MaybeClassifyAISerp(const GURL& url);

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_AI_SERP_CLASSIFIER_UTIL_H_
