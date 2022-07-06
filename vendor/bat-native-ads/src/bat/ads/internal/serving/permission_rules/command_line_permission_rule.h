/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_PERMISSION_RULES_COMMAND_LINE_PERMISSION_RULE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_PERMISSION_RULES_COMMAND_LINE_PERMISSION_RULE_H_

#include <string>

#include "bat/ads/internal/serving/permission_rules/permission_rule_interface.h"

namespace ads {

class CommandLinePermissionRule final : public PermissionRuleInterface {
 public:
  CommandLinePermissionRule();
  ~CommandLinePermissionRule() override;

  bool ShouldAllow() override;

  std::string GetLastMessage() const override;

 private:
  bool DoesRespectCap();

  CommandLinePermissionRule(const CommandLinePermissionRule&) = delete;
  CommandLinePermissionRule& operator=(const CommandLinePermissionRule&) =
      delete;

  std::string last_message_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_PERMISSION_RULES_COMMAND_LINE_PERMISSION_RULE_H_
