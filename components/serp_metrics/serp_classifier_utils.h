/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_SERP_CLASSIFIER_UTILS_H_
#define BRAVE_COMPONENTS_SERP_METRICS_SERP_CLASSIFIER_UTILS_H_

#include "components/search_engines/search_engine_type.h"

class GURL;

namespace serp_metrics {

// Returns `true` if the search engine type is allowed to be classified.
bool IsAllowedSearchEngine(SearchEngineType type);

// Returns `true` if `url` is a Google web search. Returns `false` if `tbm` is
// present (a vertical search such as images, news, or video) or `udm` is
// present with a non-zero value (a vertical such as shopping). `udm=0` and
// `udm=28` are both treated as web searches.
bool IsGoogleWebSearch(const GURL& url);

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_SERP_CLASSIFIER_UTILS_H_
