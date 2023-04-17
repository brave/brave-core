/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_EXCLUSION_RULES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_EXCLUSION_RULES_H_

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rules_base.h"

namespace brave_ads {

namespace resource {
class AntiTargeting;
}  // namespace resource

class SubdivisionTargeting;

namespace new_tab_page_ads {

class ExclusionRules final : public ExclusionRulesBase {
 public:
  ExclusionRules(const AdEventList& ad_events,
                 const SubdivisionTargeting& subdivision_targeting,
                 const resource::AntiTargeting& anti_targeting_resource,
                 const BrowsingHistoryList& browsing_history);
};

}  // namespace new_tab_page_ads
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_EXCLUSION_RULES_H_
