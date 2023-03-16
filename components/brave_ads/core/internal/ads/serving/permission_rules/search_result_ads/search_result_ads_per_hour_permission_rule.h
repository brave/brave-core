/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_PERMISSION_RULES_SEARCH_RESULT_ADS_SEARCH_RESULT_ADS_PER_HOUR_PERMISSION_RULE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_PERMISSION_RULES_SEARCH_RESULT_ADS_SEARCH_RESULT_ADS_PER_HOUR_PERMISSION_RULE_H_

#include <string>

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rule_interface.h"

namespace brave_ads::search_result_ads {

class AdsPerHourPermissionRule final : public PermissionRuleInterface {
 public:
  bool ShouldAllow() override;

  const std::string& GetLastMessage() const override;

 private:
  std::string last_message_;
};

}  // namespace brave_ads::search_result_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_PERMISSION_RULES_SEARCH_RESULT_ADS_SEARCH_RESULT_ADS_PER_HOUR_PERMISSION_RULE_H_
