// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/traffic_control/core/browser/traffic_rule_matcher.h"

#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>

#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/types/optional_ref.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom.h"
#include "components/url_matcher/url_matcher.h"
#include "components/url_matcher/url_util.h"
#include "url/gurl.h"

namespace traffic_control {

TrafficRuleMatcher::TrafficRuleMatcher() = default;

TrafficRuleMatcher::~TrafficRuleMatcher() = default;

void TrafficRuleMatcher::Rebuild(std::vector<mojom::TrafficRulePtr> rules) {
  url_matcher_ = std::make_unique<url_matcher::URLMatcher>();
  pattern_id_to_rule_index_.clear();
  pattern_id_to_rule_index_.reserve(rules.size());
  rules_ = std::move(rules);

  url_matcher::URLMatcherConditionSet::Vector all_conditions;

  for (size_t rule_index = 0; rule_index < rules_.size(); ++rule_index) {
    const auto& rule = rules_[rule_index];
    if (!rule || !rule->enabled || !rule->condition ||
        !rule->condition->url_filter.has_value()) {
      continue;
    }

    for (std::string_view line :
         base::SplitStringPiece(*rule->condition->url_filter, "\n",
                                base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL)) {
      line = base::TrimWhitespaceASCII(line, base::TRIM_ALL);
      if (line.empty() || line.starts_with('#')) {
        continue;
      }

      url_matcher::util::FilterComponents components;
      if (!url_matcher::util::FilterToComponents(
              std::string(line), &components.scheme, &components.host,
              &components.match_subdomains, &components.port, &components.path,
              &components.query)) {
        LOG(ERROR) << "Invalid traffic control URL filter: " << line;
        continue;
      }

      const base::MatcherStringPattern::ID pattern_id =
          pattern_id_to_rule_index_.size();
      all_conditions.push_back(url_matcher::util::CreateConditionSet(
          url_matcher_.get(), pattern_id, components.scheme, components.host,
          components.match_subdomains, components.port, components.path,
          components.query, /*allow=*/true));
      pattern_id_to_rule_index_.push_back(rule_index);
    }
  }

  if (!all_conditions.empty()) {
    url_matcher_->AddConditionSets(all_conditions);
  }
}

base::optional_ref<const mojom::TrafficRule>
TrafficRuleMatcher::FindMatchingRule(const GURL& url) const {
  if (!url_matcher_ || !url.is_valid()) {
    return std::nullopt;
  }

  const std::set<base::MatcherStringPattern::ID> matches =
      url_matcher_->MatchURL(url);
  if (matches.empty()) {
    return std::nullopt;
  }

  std::optional<size_t> best_index;
  for (const base::MatcherStringPattern::ID pattern_id : matches) {
    if (pattern_id >= pattern_id_to_rule_index_.size()) {
      continue;
    }
    const size_t rule_index = pattern_id_to_rule_index_[pattern_id];
    if (!best_index || rule_index < *best_index) {
      best_index = rule_index;
    }
  }

  if (!best_index || *best_index >= rules_.size() || !rules_[*best_index]) {
    return std::nullopt;
  }
  return *rules_[*best_index];
}

}  // namespace traffic_control
