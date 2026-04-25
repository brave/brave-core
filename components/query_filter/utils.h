// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_QUERY_FILTER_UTILS_H_
#define BRAVE_COMPONENTS_QUERY_FILTER_UTILS_H_

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "url/gurl.h"

namespace query_filter {
std::optional<GURL> ApplyQueryFilter(const GURL& original_url);

// This function will return a new url stripping known tracking query params.
// If nothing is to be stripped, a null value is returned.
//
// `initiator_url` specifies the origin initiating the resource request.
// If there were redirects, this is the url prior to any redirects.
// `redirect_source_url` specifies the url that we are currently navigating
// from, including any redirects that might have happened. `request_url`
// specifies where we are navigating to. `request_method` indicates the HTTP
// method of the request. `internal_redirect` indicates wether or not this is an
// internal redirect or not. This function returns the url we should redirect to
// or a `std::nullopt` value if nothing is changed.
std::optional<GURL> MaybeApplyQueryStringFilter(
    const GURL& initiator_url,
    const GURL& redirect_source_url,
    const GURL& request_url,
    const std::string& request_method,
    const bool internal_redirect);

bool IsScopedTrackerForTesting(
    std::string_view param_name,
    std::string_view spec,
    const std::map<std::string_view, std::vector<std::string_view>>& trackers);
}  // namespace query_filter
#endif  // BRAVE_COMPONENTS_QUERY_FILTER_UTILS_H_
