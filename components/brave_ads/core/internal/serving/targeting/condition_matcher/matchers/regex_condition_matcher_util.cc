/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/regex_condition_matcher_util.h"

#include "third_party/re2/src/re2/re2.h"

namespace brave_ads {

bool MatchRegex(std::string_view value, std::string_view condition) {
  const re2::RE2 re(condition, re2::RE2::Quiet);
  if (!re.ok()) {
    return false;
  }

  return re2::RE2::PartialMatch(value, re);
}

}  // namespace brave_ads
