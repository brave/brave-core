/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_SERP_CLASSIFIER_H_
#define BRAVE_COMPONENTS_SERP_METRICS_SERP_CLASSIFIER_H_

#include <optional>

#include "components/search_engines/search_engine_type.h"

class GURL;

// SERP classifier determines whether a URL is a search engine results page and,
// if so, identifies the corresponding search engine.

namespace serp_metrics {

// Returns `true` if `lhs` and `rhs` represent the same search results page.
bool IsSameSearchQuery(const GURL& lhs, const GURL& rhs);

// Returns the corresponding search engine type if `url` is a SERP. Returns
// `std::nullopt` if `url` is not a SERP.
std::optional<SearchEngineType> MaybeClassifySearchEngine(const GURL& url);

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_SERP_CLASSIFIER_H_
