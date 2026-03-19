/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/prefs/internal/condition_matcher_keyword_path_component_util.h"

namespace brave_ads {

std::optional<std::string_view> MaybeParseKeywordPathComponentValue(
    std::string_view path_component,
    std::string_view keyword) {
  if (!path_component.starts_with(keyword)) {
    return std::nullopt;
  }
  std::string_view remainder = path_component.substr(keyword.size());
  if (remainder.empty()) {
    return remainder;
  }
  if (!remainder.starts_with('=')) {
    // Something follows the keyword but is not separated by '='.
    return std::nullopt;
  }
  return remainder.substr(1);
}

}  // namespace brave_ads
