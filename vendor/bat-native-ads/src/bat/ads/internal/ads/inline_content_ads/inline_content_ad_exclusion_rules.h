/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EXCLUSION_RULES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EXCLUSION_RULES_H_

#include "bat/ads/internal/ads/exclusion_rules_base.h"

namespace ads {

namespace ad_targeting {
namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic
}  // namespace ad_targeting

namespace resource {
class AntiTargeting;
}  // namespace resource

namespace inline_content_ads {
namespace frequency_capping {

class ExclusionRules final : public ExclusionRulesBase {
 public:
  ExclusionRules(
      const AdEventList& ad_events,
      ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
      resource::AntiTargeting* anti_targeting_resource,
      const BrowsingHistoryList& browsing_history);
  ~ExclusionRules() override;

 private:
  ExclusionRules(const ExclusionRules&) = delete;
  ExclusionRules& operator=(const ExclusionRules&) = delete;
};

}  // namespace frequency_capping
}  // namespace inline_content_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EXCLUSION_RULES_H_
