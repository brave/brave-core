/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/internal/numerical_operator_condition_matcher_util_internal.h"

#include <cstddef>
#include <string>

#include "base/check.h"
#include "base/logging.h"
#include "base/strings/pattern.h"
#include "base/strings/string_number_conversions.h"

namespace brave_ads {

std::optional<double> ParseNumber(const std::string_view condition) {
  CHECK(base::MatchPattern(condition,
                           kNumericalOperatorConditionMatcherPrefixPattern));

  const size_t pos = condition.find(':');
  if (pos == std::string::npos || pos + 1 >= condition.size()) {
    // Malformed operator.
    VLOG(1) << "Malformed numerical operator condition matcher for "
            << condition;
    return std::nullopt;
  }

  double number;
  if (!base::StringToDouble(condition.substr(pos + 1), &number)) {
    // Malformed number.
    VLOG(1) << "Malformed numerical operator condition matcher for "
            << condition;
    return std::nullopt;
  }

  return number;
}

}  // namespace brave_ads
