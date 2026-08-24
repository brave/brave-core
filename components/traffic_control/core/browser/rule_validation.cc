// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/traffic_control/core/browser/rule_validation.h"

#include <string>
#include <string_view>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom.h"
#include "components/url_matcher/url_util.h"

namespace traffic_control {

namespace {

bool IsValidUrlFilterLine(std::string_view filter) {
  std::string scheme;
  std::string host;
  bool match_subdomains = false;
  uint16_t port = 0;
  std::string path;
  std::string query;
  return url_matcher::util::FilterToComponents(std::string(filter), &scheme,
                                               &host, &match_subdomains, &port,
                                               &path, &query);
}

}  // namespace

bool IsValidUrlFilter(std::string_view filter_text) {
  // One pattern per line. Blank lines and `#` comments are skipped. Any other
  // line that fails to parse makes the whole filter invalid. At least one
  // valid pattern is required.
  bool has_pattern = false;
  for (std::string_view line : base::SplitStringPiece(
           filter_text, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL)) {
    line = base::TrimWhitespaceASCII(line, base::TRIM_ALL);
    if (line.empty() || line.starts_with('#')) {
      continue;
    }
    if (!IsValidUrlFilterLine(line)) {
      return false;
    }
    has_pattern = true;
  }
  return has_pattern;
}

std::optional<mojom::RuleOperationError> ValidateRule(
    const mojom::TrafficRulePtr& rule,
    bool require_empty_id) {
  if (!rule || !rule->target) {
    return mojom::RuleOperationError::kInvalidTarget;
  }
  if (require_empty_id) {
    if (!rule->id.empty()) {
      return mojom::RuleOperationError::kIdShouldBeEmpty;
    }
  } else if (rule->id.empty()) {
    return mojom::RuleOperationError::kIdShouldBeSet;
  }
  if (!rule->condition) {
    return mojom::RuleOperationError::kInvalidCondition;
  }

  if (!rule->condition->url_filter.has_value() ||
      !IsValidUrlFilter(*rule->condition->url_filter)) {
    return mojom::RuleOperationError::kInvalidUrlFilter;
  }

  // temporary_container and container_id are mutually exclusive.
  if (rule->target->temporary_container &&
      rule->target->container_id.has_value()) {
    return mojom::RuleOperationError::kInvalidTarget;
  }

  return std::nullopt;
}

}  // namespace traffic_control
