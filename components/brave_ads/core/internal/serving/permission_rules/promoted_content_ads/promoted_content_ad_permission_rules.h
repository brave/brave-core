/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_PERMISSION_RULES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_PERMISSION_RULES_H_

#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules_base.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"

namespace brave_ads {

class PromotedContentAdPermissionRules final : public PermissionRulesBase {
 public:
  static bool HasPermission(const AdEventList& ad_events);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_PERMISSION_RULES_H_
