/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_PERMISSION_RULES_USER_ACTIVITY_PERMISSION_RULE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_PERMISSION_RULES_USER_ACTIVITY_PERMISSION_RULE_H_

#include <string>

#include "bat/ads/internal/frequency_capping/permission_rules/permission_rule.h"

namespace ads {

class UserActivityPermissionRule final : public PermissionRule {
 public:
  UserActivityPermissionRule();
  ~UserActivityPermissionRule() override;

  bool ShouldAllow() override;

  std::string GetLastMessage() const override;

 private:
  std::string last_message_;

  bool DoesRespectCap();

  UserActivityPermissionRule(const UserActivityPermissionRule&) = delete;
  UserActivityPermissionRule& operator=(const UserActivityPermissionRule&) =
      delete;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_PERMISSION_RULES_USER_ACTIVITY_PERMISSION_RULE_H_
