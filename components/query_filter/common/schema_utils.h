// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_QUERY_FILTER_COMMON_SCHEMA_UTILS_H_
#define BRAVE_COMPONENTS_QUERY_FILTER_COMMON_SCHEMA_UTILS_H_

#include <string>
#include <string_view>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "brave/components/query_filter/common/schema.h"

namespace query_filter {
// Parses through the |rules| to find all the schema::Rule that includes
// |spec| and returns an assimilated set of params which should be stripped
// away for that |spec|.
absl::flat_hash_set<std::string> GetBlocklistedParamsForSpec(
    const std::vector<schema::Rule>& rules,
    std::string_view spec);
}  // namespace query_filter

#endif  // BRAVE_COMPONENTS_QUERY_FILTER_COMMON_SCHEMA_UTILS_H_
