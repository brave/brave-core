/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_EXCLUSION_RULES_BASE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_EXCLUSION_RULES_BASE_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "bat/ads/internal/ads/ad_events/ad_event_info.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_alias.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_interface.h"

namespace ads {

namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic

namespace resource {
class AntiTargeting;
}  // namespace resource

class AntiTargetingExclusionRule;
class ConversionExclusionRule;
class DailyCapExclusionRule;
class DaypartExclusionRule;
class DislikeExclusionRule;
class MarkedAsInappropriateExclusionRule;
class MarkedToNoLongerReceiveExclusionRule;
class PerDayExclusionRule;
class PerMonthExclusionRule;
class PerWeekExclusionRule;
class SplitTestExclusionRule;
class SubdivisionTargetingExclusionRule;
class TotalMaxExclusionRule;
class TransferredExclusionRule;
struct CreativeAdInfo;

class ExclusionRulesBase {
 public:
  ExclusionRulesBase(const ExclusionRulesBase& other) = delete;
  ExclusionRulesBase& operator=(const ExclusionRulesBase& other) = delete;

  ExclusionRulesBase(ExclusionRulesBase&& other) noexcept = delete;
  ExclusionRulesBase& operator=(ExclusionRulesBase&& other) noexcept = delete;

  virtual ~ExclusionRulesBase();

  virtual bool ShouldExcludeCreativeAd(const CreativeAdInfo& creative_ad);

 protected:
  ExclusionRulesBase(const AdEventList& ad_events,
                     geographic::SubdivisionTargeting* subdivision_targeting,
                     resource::AntiTargeting* anti_targeting_resource,
                     const BrowsingHistoryList& browsing_history);

  std::vector<ExclusionRuleInterface<CreativeAdInfo>*> exclusion_rules_;

  std::set<std::string> uuids_;
  bool AddToCacheIfNeeded(
      const CreativeAdInfo& creative_ad,
      ExclusionRuleInterface<CreativeAdInfo>* exclusion_rule);

 private:
  bool IsCached(const CreativeAdInfo& creative_ad) const;
  void AddToCache(const std::string& uuid);

  std::unique_ptr<AntiTargetingExclusionRule> anti_targeting_exclusion_rule_;
  std::unique_ptr<ConversionExclusionRule> conversion_exclusion_rule_;
  std::unique_ptr<DailyCapExclusionRule> daily_cap_exclusion_rule_;
  std::unique_ptr<DaypartExclusionRule> daypart_exclusion_rule_;
  std::unique_ptr<DislikeExclusionRule> dislike_exclusion_rule_;
  std::unique_ptr<MarkedAsInappropriateExclusionRule>
      marked_as_inappropriate_exclusion_rule_;
  std::unique_ptr<MarkedToNoLongerReceiveExclusionRule>
      marked_to_no_longer_receive_exclusion_rule_;
  std::unique_ptr<PerDayExclusionRule> per_day_exclusion_rule_;
  std::unique_ptr<PerMonthExclusionRule> per_month_exclusion_rule_;
  std::unique_ptr<PerWeekExclusionRule> per_week_exclusion_rule_;
  std::unique_ptr<SplitTestExclusionRule> split_test_exclusion_rule_;
  std::unique_ptr<SubdivisionTargetingExclusionRule>
      subdivision_targeting_exclusion_rule_;
  std::unique_ptr<TotalMaxExclusionRule> total_max_exclusion_rule_;
  std::unique_ptr<TransferredExclusionRule> transferred_exclusion_rule_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_EXCLUSION_RULES_BASE_H_
