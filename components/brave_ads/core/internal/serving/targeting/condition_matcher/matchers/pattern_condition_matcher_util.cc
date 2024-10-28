/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/pattern_condition_matcher_util.h"

#include "base/strings/pattern.h"

namespace brave_ads {

bool MatchPattern(const std::string_view value,
                  const std::string_view condition) {
  return base::MatchPattern(value, condition);
}

}  // namespace brave_ads
