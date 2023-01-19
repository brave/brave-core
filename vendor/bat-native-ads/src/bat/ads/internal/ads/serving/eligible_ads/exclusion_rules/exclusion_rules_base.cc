/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rules_base.h"

#include "base/ranges/algorithm.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/anti_targeting_exclusion_rule.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/conversion_exclusion_rule.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/daily_cap_exclusion_rule.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/daypart_exclusion_rule.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/dislike_exclusion_rule.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/marked_as_inappropriate_exclusion_rule.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/marked_to_no_longer_receive_exclusion_rule.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/per_day_exclusion_rule.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/per_month_exclusion_rule.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/per_week_exclusion_rule.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/split_test_exclusion_rule.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/subdivision_targeting_exclusion_rule.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/total_max_exclusion_rule.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/transferred_exclusion_rule.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/creatives/creative_ad_info.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

namespace ads {

ExclusionRulesBase::ExclusionRulesBase(
    const AdEventList& ad_events,
    geographic::SubdivisionTargeting* subdivision_targeting,
    resource::AntiTargeting* anti_targeting_resource,
    const BrowsingHistoryList& browsing_history) {
  DCHECK(subdivision_targeting);
  DCHECK(anti_targeting_resource);

  split_test_exclusion_rule_ = std::make_unique<SplitTestExclusionRule>();
  exclusion_rules_.push_back(split_test_exclusion_rule_.get());

  subdivision_targeting_exclusion_rule_ =
      std::make_unique<SubdivisionTargetingExclusionRule>(
          subdivision_targeting);
  exclusion_rules_.push_back(subdivision_targeting_exclusion_rule_.get());

  anti_targeting_exclusion_rule_ = std::make_unique<AntiTargetingExclusionRule>(
      anti_targeting_resource, browsing_history);
  exclusion_rules_.push_back(anti_targeting_exclusion_rule_.get());

  dislike_exclusion_rule_ = std::make_unique<DislikeExclusionRule>();
  exclusion_rules_.push_back(dislike_exclusion_rule_.get());

  marked_as_inappropriate_exclusion_rule_ =
      std::make_unique<MarkedAsInappropriateExclusionRule>();
  exclusion_rules_.push_back(marked_as_inappropriate_exclusion_rule_.get());

  marked_to_no_longer_receive_exclusion_rule_ =
      std::make_unique<MarkedToNoLongerReceiveExclusionRule>();
  exclusion_rules_.push_back(marked_to_no_longer_receive_exclusion_rule_.get());

  conversion_exclusion_rule_ =
      std::make_unique<ConversionExclusionRule>(ad_events);
  exclusion_rules_.push_back(conversion_exclusion_rule_.get());

  transferred_exclusion_rule_ =
      std::make_unique<TransferredExclusionRule>(ad_events);
  exclusion_rules_.push_back(transferred_exclusion_rule_.get());

  total_max_exclusion_rule_ =
      std::make_unique<TotalMaxExclusionRule>(ad_events);
  exclusion_rules_.push_back(total_max_exclusion_rule_.get());

  per_month_exclusion_rule_ =
      std::make_unique<PerMonthExclusionRule>(ad_events);
  exclusion_rules_.push_back(per_month_exclusion_rule_.get());

  per_week_exclusion_rule_ = std::make_unique<PerWeekExclusionRule>(ad_events);
  exclusion_rules_.push_back(per_week_exclusion_rule_.get());

  daily_cap_exclusion_rule_ =
      std::make_unique<DailyCapExclusionRule>(ad_events);
  exclusion_rules_.push_back(daily_cap_exclusion_rule_.get());

  per_day_exclusion_rule_ = std::make_unique<PerDayExclusionRule>(ad_events);
  exclusion_rules_.push_back(per_day_exclusion_rule_.get());

  daypart_exclusion_rule_ = std::make_unique<DaypartExclusionRule>();
  exclusion_rules_.push_back(daypart_exclusion_rule_.get());
}

ExclusionRulesBase::~ExclusionRulesBase() = default;

bool ExclusionRulesBase::ShouldExcludeCreativeAd(
    const CreativeAdInfo& creative_ad) {
  return base::ranges::any_of(exclusion_rules_, [=](auto* exclusion_rule) {
    return AddToCacheIfNeeded(creative_ad, exclusion_rule);
  });
}

bool ExclusionRulesBase::AddToCacheIfNeeded(
    const CreativeAdInfo& creative_ad,
    ExclusionRuleInterface<CreativeAdInfo>* exclusion_rule) {
  DCHECK(exclusion_rule);

  if (IsCached(creative_ad)) {
    return true;
  }

  if (!exclusion_rule->ShouldExclude(creative_ad)) {
    return false;
  }

  const std::string& last_message = exclusion_rule->GetLastMessage();
  if (!last_message.empty()) {
    BLOG(2, last_message);
  }

  const std::string uuid = exclusion_rule->GetUuid(creative_ad);
  AddToCache(uuid);

  return true;
}

bool ExclusionRulesBase::IsCached(const CreativeAdInfo& creative_ad) const {
  if (uuids_.find(creative_ad.campaign_id) != uuids_.cend()) {
    return true;
  }

  if (uuids_.find(creative_ad.advertiser_id) != uuids_.cend()) {
    return true;
  }

  if (uuids_.find(creative_ad.creative_set_id) != uuids_.cend()) {
    return true;
  }

  if (uuids_.find(creative_ad.creative_instance_id) != uuids_.cend()) {
    return true;
  }

  if (uuids_.find(creative_ad.segment) != uuids_.cend()) {
    return true;
  }

  return false;
}

void ExclusionRulesBase::AddToCache(const std::string& uuid) {
  uuids_.insert(uuid);
}

}  // namespace ads
