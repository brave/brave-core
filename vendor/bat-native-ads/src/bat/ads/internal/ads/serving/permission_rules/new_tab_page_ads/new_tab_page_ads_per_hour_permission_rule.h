/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_PERMISSION_RULES_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_ADS_PER_HOUR_PERMISSION_RULE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_PERMISSION_RULES_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_ADS_PER_HOUR_PERMISSION_RULE_H_

#include <string>
#include <vector>

#include "bat/ads/internal/ads/serving/permission_rules/permission_rule_interface.h"

namespace base {
class Time;
}  // namespace base

namespace ads::new_tab_page_ads {

class AdsPerHourPermissionRule final : public PermissionRuleInterface {
 public:
  AdsPerHourPermissionRule();

  AdsPerHourPermissionRule(const AdsPerHourPermissionRule&) = delete;
  AdsPerHourPermissionRule& operator=(const AdsPerHourPermissionRule&) = delete;

  AdsPerHourPermissionRule(AdsPerHourPermissionRule&& other) noexcept = delete;
  AdsPerHourPermissionRule& operator=(
      AdsPerHourPermissionRule&& other) noexcept = delete;

  ~AdsPerHourPermissionRule() override;

  bool ShouldAllow() override;

  const std::string& GetLastMessage() const override;

 private:
  bool DoesRespectCap(const std::vector<base::Time>& history);

  std::string last_message_;
};

}  // namespace ads::new_tab_page_ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_PERMISSION_RULES_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_ADS_PER_HOUR_PERMISSION_RULE_H_
