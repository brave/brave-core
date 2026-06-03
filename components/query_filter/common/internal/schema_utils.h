// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_QUERY_FILTER_COMMON_INTERNAL_SCHEMA_UTILS_H_
#define BRAVE_COMPONENTS_QUERY_FILTER_COMMON_INTERNAL_SCHEMA_UTILS_H_

#include <string>
#include <string_view>

#include "base/containers/flat_set.h"
#include "base/containers/span.h"
#include "brave/components/query_filter/common/schema.h"

namespace query_filter {
// Given a set of filter |rules| and a URL |spec|, returns the union of all
// query parameters that should be stripped from |spec|.
//
// Each rule carries three lists: |include| patterns (glob-style, e.g.
// "*://*.example.com/*"), |exclude| patterns, and |params| to strip. A rule
// contributes its params only when |spec| matches at least one include pattern
// and no exclude pattern. Blank/whitespace-only patterns are treated as
// non-matching. Returns an empty set if |spec| is not a valid URL or no rule
// matches.
base::flat_set<std::string> GetBlocklistedParamsForSpec(
    base::span<const schema::Rule> rules,
    std::string_view spec);
}  // namespace query_filter

#endif  // BRAVE_COMPONENTS_QUERY_FILTER_COMMON_INTERNAL_SCHEMA_UTILS_H_
