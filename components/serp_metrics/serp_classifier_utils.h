/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_SERP_CLASSIFIER_UTILS_H_
#define BRAVE_COMPONENTS_SERP_METRICS_SERP_CLASSIFIER_UTILS_H_

#include "components/search_engines/search_engine_type.h"

namespace serp_metrics {

// Returns `true` if the search engine type is allowed to be classified.
bool IsAllowedSearchEngine(SearchEngineType type);

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_SERP_CLASSIFIER_UTILS_H_
