/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/exclusion_rules_base.h"

#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/anti_targeting_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/conversion_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/daily_cap_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/daypart_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/dislike_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/exclusion_rule.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/exclusion_rule_util.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/marked_as_inappropriate_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/marked_to_no_longer_receive_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/per_day_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/per_hour_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/per_month_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/per_week_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/split_test_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/subdivision_targeting_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/total_max_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/transferred_frequency_cap.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"

namespace ads {

ExclusionRulesBase::ExclusionRulesBase(
    const AdEventList& ad_events,
    ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
    resource::AntiTargeting* anti_targeting_resource,
    const BrowsingHistoryList& browsing_history) {
  DCHECK(subdivision_targeting);
  DCHECK(anti_targeting_resource);

  split_test_frequency_cap_ = std::make_unique<SplitTestFrequencyCap>();
  exclusion_rules_.push_back(split_test_frequency_cap_.get());

  subdivision_targeting_frequency_cap_ =
      std::make_unique<SubdivisionTargetingFrequencyCap>(subdivision_targeting);
  exclusion_rules_.push_back(subdivision_targeting_frequency_cap_.get());

  anti_targeting_frequency_cap_ = std::make_unique<AntiTargetingFrequencyCap>(
      anti_targeting_resource, browsing_history);
  exclusion_rules_.push_back(anti_targeting_frequency_cap_.get());

  dislike_frequency_cap_ = std::make_unique<DislikeFrequencyCap>();
  exclusion_rules_.push_back(dislike_frequency_cap_.get());

  marked_as_inappropriate_frequency_cap_ =
      std::make_unique<MarkedAsInappropriateFrequencyCap>();
  exclusion_rules_.push_back(marked_as_inappropriate_frequency_cap_.get());

  marked_to_no_longer_receive_frequency_cap_ =
      std::make_unique<MarkedToNoLongerReceiveFrequencyCap>();
  exclusion_rules_.push_back(marked_to_no_longer_receive_frequency_cap_.get());

  conversion_frequency_cap_ =
      std::make_unique<ConversionFrequencyCap>(ad_events);
  exclusion_rules_.push_back(conversion_frequency_cap_.get());

  transferred_frequency_cap_ =
      std::make_unique<TransferredFrequencyCap>(ad_events);
  exclusion_rules_.push_back(transferred_frequency_cap_.get());

  total_max_frequency_cap_ = std::make_unique<TotalMaxFrequencyCap>(ad_events);
  exclusion_rules_.push_back(total_max_frequency_cap_.get());

  per_month_frequency_cap_ = std::make_unique<PerMonthFrequencyCap>(ad_events);
  exclusion_rules_.push_back(per_month_frequency_cap_.get());

  per_week_frequency_cap_ = std::make_unique<PerWeekFrequencyCap>(ad_events);
  exclusion_rules_.push_back(per_week_frequency_cap_.get());

  daily_cap_frequency_cap_ = std::make_unique<DailyCapFrequencyCap>(ad_events);
  exclusion_rules_.push_back(daily_cap_frequency_cap_.get());

  per_day_frequency_cap_ = std::make_unique<PerDayFrequencyCap>(ad_events);
  exclusion_rules_.push_back(per_day_frequency_cap_.get());

  daypart_frequency_cap_ = std::make_unique<DaypartFrequencyCap>();
  exclusion_rules_.push_back(daypart_frequency_cap_.get());

  per_hour_frequency_cap_ = std::make_unique<PerHourFrequencyCap>(ad_events);
  exclusion_rules_.push_back(per_hour_frequency_cap_.get());
}

ExclusionRulesBase::~ExclusionRulesBase() = default;

bool ExclusionRulesBase::ShouldExcludeCreativeAd(
    const CreativeAdInfo& creative_ad) {
  for (auto* exclusion_rule : exclusion_rules_) {
    DCHECK(exclusion_rule);

    if (AddToCacheIfNeeded(creative_ad, exclusion_rule)) {
      return true;
    }
  }

  return false;
}

///////////////////////////////////////////////////////////////////////////////

bool ExclusionRulesBase::AddToCacheIfNeeded(
    const CreativeAdInfo& creative_ad,
    ExclusionRule<CreativeAdInfo>* exclusion_rule) {
  DCHECK(exclusion_rule);

  if (IsCached(creative_ad)) {
    return true;
  }

  if (!exclusion_rule->ShouldExclude(creative_ad)) {
    return false;
  }

  const std::string last_message = exclusion_rule->GetLastMessage();
  if (!last_message.empty()) {
    BLOG(2, last_message);
  }

  const std::string uuid = exclusion_rule->GetUuid(creative_ad);
  AddToCache(uuid);

  return true;
}

bool ExclusionRulesBase::IsCached(const CreativeAdInfo& creative_ad) const {
  if (uuids_.find(creative_ad.campaign_id) != uuids_.end()) {
    return true;
  }

  if (uuids_.find(creative_ad.advertiser_id) != uuids_.end()) {
    return true;
  }

  if (uuids_.find(creative_ad.creative_set_id) != uuids_.end()) {
    return true;
  }

  if (uuids_.find(creative_ad.creative_instance_id) != uuids_.end()) {
    return true;
  }

  return false;
}

void ExclusionRulesBase::AddToCache(const std::string& uuid) {
  uuids_.insert(uuid);
}

}  // namespace ads
