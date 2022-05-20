/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_SUBDIVISION_TARGETING_EXCLUSION_RULE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_SUBDIVISION_TARGETING_EXCLUSION_RULE_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "bat/ads/internal/creatives/creative_ad_info.h"
#include "bat/ads/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_interface.h"

namespace ads {

namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic

class SubdivisionTargetingExclusionRule final
    : public ExclusionRuleInterface<CreativeAdInfo> {
 public:
  SubdivisionTargetingExclusionRule(
      geographic::SubdivisionTargeting* subdivision_targeting);
  ~SubdivisionTargetingExclusionRule() override;

  SubdivisionTargetingExclusionRule(const SubdivisionTargetingExclusionRule&) =
      delete;
  SubdivisionTargetingExclusionRule& operator=(
      const SubdivisionTargetingExclusionRule&) = delete;

  std::string GetUuid(const CreativeAdInfo& creative_ad) const override;

  bool ShouldExclude(const CreativeAdInfo& creative_ad) override;

  std::string GetLastMessage() const override;

 private:
  bool DoesRespectCap(const CreativeAdInfo& creative_ad);

  raw_ptr<geographic::SubdivisionTargeting> subdivision_targeting_ =
      nullptr;  // NOT OWNED

  std::string last_message_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_SUBDIVISION_TARGETING_EXCLUSION_RULE_H_
