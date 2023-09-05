/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_PERMISSION_RULE_INTERFACE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_PERMISSION_RULE_INTERFACE_H_

#include <string>

#include "base/types/expected.h"

namespace brave_ads {

class PermissionRuleInterface {
 public:
  virtual ~PermissionRuleInterface() = default;

  virtual base::expected<void, std::string> ShouldAllow() const = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_PERMISSION_RULE_INTERFACE_H_
