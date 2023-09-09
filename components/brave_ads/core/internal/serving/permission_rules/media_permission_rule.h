/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_MEDIA_PERMISSION_RULE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_MEDIA_PERMISSION_RULE_H_

#include <string>

#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rule_interface.h"

namespace brave_ads {

class MediaPermissionRule final : public PermissionRuleInterface {
 public:
  base::expected<void, std::string> ShouldAllow() const override;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_MEDIA_PERMISSION_RULE_H_
