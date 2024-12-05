/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EXCLUSION_RULES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EXCLUSION_RULES_H_

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rules_base.h"

namespace brave_ads {

class AntiTargetingResource;
class SubdivisionTargeting;

class InlineContentAdExclusionRules final : public ExclusionRulesBase {
 public:
  InlineContentAdExclusionRules(
      const AdEventList& ad_events,
      const SubdivisionTargeting& subdivision_targeting,
      const AntiTargetingResource& anti_targeting_resource,
      const SiteHistoryList& site_history);

  InlineContentAdExclusionRules(const InlineContentAdExclusionRules&) = delete;
  InlineContentAdExclusionRules& operator=(
      const InlineContentAdExclusionRules&) = delete;

  ~InlineContentAdExclusionRules() override;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EXCLUSION_RULES_H_
