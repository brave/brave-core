/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Serializes and deserializes a ConditionMatcherMap for storage in a single
// database column string.

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONDITION_MATCHERS_DATABASE_TABLE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONDITION_MATCHERS_DATABASE_TABLE_UTIL_H_

#include <string>

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/condition_matcher_util.h"

namespace brave_ads::database::table {

std::string ConditionMatchersToString(
    const ConditionMatcherMap& condition_matchers);

ConditionMatcherMap StringToConditionMatchers(const std::string& value);

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONDITION_MATCHERS_DATABASE_TABLE_UTIL_H_
