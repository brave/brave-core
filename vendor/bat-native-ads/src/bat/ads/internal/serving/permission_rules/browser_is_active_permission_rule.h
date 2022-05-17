/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_PERMISSION_RULES_BROWSER_IS_ACTIVE_PERMISSION_RULE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_PERMISSION_RULES_BROWSER_IS_ACTIVE_PERMISSION_RULE_H_

#include <string>

#include "bat/ads/internal/serving/permission_rules/permission_rule_interface.h"

namespace ads {

class BrowserIsActivePermissionRule final : public PermissionRuleInterface {
 public:
  BrowserIsActivePermissionRule();
  ~BrowserIsActivePermissionRule() override;

  bool ShouldAllow() override;

  std::string GetLastMessage() const override;

 private:
  bool DoesRespectCap();

  BrowserIsActivePermissionRule(const BrowserIsActivePermissionRule&) = delete;
  BrowserIsActivePermissionRule& operator=(
      const BrowserIsActivePermissionRule&) = delete;

  std::string last_message_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_PERMISSION_RULES_BROWSER_IS_ACTIVE_PERMISSION_RULE_H_
