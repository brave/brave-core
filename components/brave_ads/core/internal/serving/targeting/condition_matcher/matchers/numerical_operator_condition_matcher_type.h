/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_MATCHERS_NUMERICAL_OPERATOR_CONDITION_MATCHER_TYPE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_MATCHERS_NUMERICAL_OPERATOR_CONDITION_MATCHER_TYPE_H_

namespace brave_ads {

enum class ConditionMatcherNumericalOperatorType {
  kEqual,
  kNotEqual,
  kGreaterThan,
  kGreaterThanOrEqual,
  kLessThan,
  kLessThanOrEqual
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_MATCHERS_NUMERICAL_OPERATOR_CONDITION_MATCHER_TYPE_H_
