// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_TRAFFIC_RULE_MATCHER_H_
#define BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_TRAFFIC_RULE_MATCHER_H_

#include <memory>
#include <vector>

#include "base/types/optional_ref.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom-forward.h"

class GURL;

namespace url_matcher {
class URLMatcher;
}  // namespace url_matcher

namespace traffic_control {

// Builds a URLMatcher from traffic control rules and finds the first matching
// enabled rule for a URL. No PrefService dependency — rebuild from a rule
// snapshot whenever prefs change.
class TrafficRuleMatcher {
 public:
  TrafficRuleMatcher();
  ~TrafficRuleMatcher();

  TrafficRuleMatcher(const TrafficRuleMatcher&) = delete;
  TrafficRuleMatcher& operator=(const TrafficRuleMatcher&) = delete;

  // Rebuilds the matcher from |rules|. Only enabled rules with a set
  // |url_filter| contribute patterns. Multiline filters are split; blank lines
  // and `#` comments are ignored. Takes |rules| by value so callers can move a
  // snapshot in without cloning.
  void Rebuild(std::vector<mojom::TrafficRulePtr> rules);

  // Returns the first matching rule (lowest list index), or empty if none
  // match. The reference is valid until the next Rebuild().
  base::optional_ref<const mojom::TrafficRule> FindMatchingRule(
      const GURL& url) const;

 private:
  std::unique_ptr<url_matcher::URLMatcher> url_matcher_;
  // Condition-set ID is the index into this vector; value is the index into
  // |rules_|.
  std::vector<size_t> pattern_id_to_rule_index_;
  std::vector<mojom::TrafficRulePtr> rules_;
};

}  // namespace traffic_control

#endif  // BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_TRAFFIC_RULE_MATCHER_H_
