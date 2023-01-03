/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_EXCLUSION_RULE_INTERFACE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_EXCLUSION_RULE_INTERFACE_H_

#include <string>

namespace ads {

template <typename T>
class ExclusionRuleInterface {
 public:
  virtual ~ExclusionRuleInterface() = default;

  virtual std::string GetUuid(const T&) const = 0;

  virtual bool ShouldExclude(const T&) = 0;

  virtual const std::string& GetLastMessage() const = 0;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_EXCLUSION_RULE_INTERFACE_H_
