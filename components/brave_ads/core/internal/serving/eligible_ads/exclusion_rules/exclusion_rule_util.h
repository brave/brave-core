/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_EXCLUSION_RULE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_EXCLUSION_RULE_UTIL_H_

#include <cstddef>

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_interface.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

struct CreativeAdInfo;

bool DoesRespectCampaignCap(const CreativeAdInfo& creative_ad,
                            const AdEventList& ad_events,
                            mojom::ConfirmationType mojom_confirmation_type,
                            base::TimeDelta time_constraint,
                            size_t cap);
bool DoesRespectCampaignCap(const CreativeAdInfo& creative_ad,
                            const AdEventList& ad_events,
                            mojom::ConfirmationType mojom_confirmation_type,
                            size_t cap);

bool DoesRespectCreativeSetCap(const CreativeAdInfo& creative_ad,
                               const AdEventList& ad_events,
                               mojom::ConfirmationType mojom_confirmation_type,
                               base::TimeDelta time_constraint,
                               size_t cap);
bool DoesRespectCreativeSetCap(const CreativeAdInfo& creative_ad,
                               const AdEventList& ad_events,
                               mojom::ConfirmationType mojom_confirmation_type,
                               size_t cap);

bool DoesRespectCreativeCap(const CreativeAdInfo& creative_ad,
                            const AdEventList& ad_events,
                            mojom::ConfirmationType mojom_confirmation_type,
                            base::TimeDelta time_constraint,
                            size_t cap);
bool DoesRespectCreativeCap(const CreativeAdInfo& creative_ad,
                            const AdEventList& ad_events,
                            mojom::ConfirmationType mojom_confirmation_type,
                            size_t cap);

template <typename T>
bool ShouldInclude(const T& ad,
                   const ExclusionRuleInterface<T>* const exclusion_rule) {
  CHECK(exclusion_rule);

  const auto result = exclusion_rule->ShouldInclude(ad);
  if (!result.has_value()) {
    BLOG(2, result.error());
    return false;
  }

  return true;
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_EXCLUSION_RULE_UTIL_H_
