/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_EXCLUSION_RULES_BASE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_EXCLUSION_RULES_BASE_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "bat/ads/internal/ad_events/ad_event_info_aliases.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_aliases.h"

namespace ads {

namespace ad_targeting {
namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic
}  // namespace ad_targeting

namespace resource {
class AntiTargeting;
}  // namespace resource

class AntiTargetingFrequencyCap;
class ConversionFrequencyCap;
class DailyCapFrequencyCap;
class DaypartFrequencyCap;
class DislikeFrequencyCap;
template <typename T>
class ExclusionRule;
class MarkedAsInappropriateFrequencyCap;
class MarkedToNoLongerReceiveFrequencyCap;
class PerDayFrequencyCap;
class PerHourFrequencyCap;
class PerMonthFrequencyCap;
class PerWeekFrequencyCap;
class SplitTestFrequencyCap;
class SubdivisionTargetingFrequencyCap;
class TotalMaxFrequencyCap;
class TransferredFrequencyCap;

class ExclusionRulesBase {
 public:
  ExclusionRulesBase(
      const AdEventList& ad_events,
      ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
      resource::AntiTargeting* anti_targeting_resource,
      const BrowsingHistoryList& browsing_history);
  virtual ~ExclusionRulesBase();

  virtual bool ShouldExcludeCreativeAd(const CreativeAdInfo& creative_ad);

 protected:
  std::vector<ExclusionRule<CreativeAdInfo>*> exclusion_rules_;

  std::set<std::string> uuids_;
  bool AddToCacheIfNeeded(const CreativeAdInfo& creative_ad,
                          ExclusionRule<CreativeAdInfo>* exclusion_rule);

 private:
  std::unique_ptr<AntiTargetingFrequencyCap> anti_targeting_frequency_cap_;
  std::unique_ptr<ConversionFrequencyCap> conversion_frequency_cap_;
  std::unique_ptr<DailyCapFrequencyCap> daily_cap_frequency_cap_;
  std::unique_ptr<DaypartFrequencyCap> daypart_frequency_cap_;
  std::unique_ptr<DislikeFrequencyCap> dislike_frequency_cap_;
  std::unique_ptr<MarkedAsInappropriateFrequencyCap>
      marked_as_inappropriate_frequency_cap_;
  std::unique_ptr<MarkedToNoLongerReceiveFrequencyCap>
      marked_to_no_longer_receive_frequency_cap_;
  std::unique_ptr<PerDayFrequencyCap> per_day_frequency_cap_;
  std::unique_ptr<PerHourFrequencyCap> per_hour_frequency_cap_;
  std::unique_ptr<PerMonthFrequencyCap> per_month_frequency_cap_;
  std::unique_ptr<PerWeekFrequencyCap> per_week_frequency_cap_;
  std::unique_ptr<SplitTestFrequencyCap> split_test_frequency_cap_;
  std::unique_ptr<SubdivisionTargetingFrequencyCap>
      subdivision_targeting_frequency_cap_;
  std::unique_ptr<TotalMaxFrequencyCap> total_max_frequency_cap_;
  std::unique_ptr<TransferredFrequencyCap> transferred_frequency_cap_;

  bool IsCached(const CreativeAdInfo& creative_ad) const;
  void AddToCache(const std::string& uuid);

  ExclusionRulesBase(const ExclusionRulesBase&) = delete;
  ExclusionRulesBase& operator=(const ExclusionRulesBase&) = delete;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_EXCLUSION_RULES_BASE_H_
