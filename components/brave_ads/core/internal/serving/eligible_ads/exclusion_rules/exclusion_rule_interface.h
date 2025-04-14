/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_EXCLUSION_RULE_INTERFACE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_EXCLUSION_RULE_INTERFACE_H_

#include <string>

namespace brave_ads {

template <typename T>
class ExclusionRuleInterface {
 public:
  virtual ~ExclusionRuleInterface() = default;

  // Return a unique cache key for the given creative ad.
  virtual std::string GetCacheKey(const T&) const = 0;

  // Returns whether the given creative ad should be included based on the
  // exclusion rule.
  virtual bool ShouldInclude(const T&) const = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_EXCLUSION_RULE_INTERFACE_H_
