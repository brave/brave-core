/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rules_base.h"

#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/anti_targeting_exclusion_rule.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/conversion_exclusion_rule.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/daily_cap_exclusion_rule.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/daypart_exclusion_rule.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/dislike_category_exclusion_rule.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/dislike_exclusion_rule.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/marked_as_inappropriate_exclusion_rule.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/per_day_exclusion_rule.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/per_month_exclusion_rule.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/per_week_exclusion_rule.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/split_test_exclusion_rule.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/subdivision_targeting_exclusion_rule.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/total_max_exclusion_rule.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/transferred_exclusion_rule.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

namespace brave_ads {

ExclusionRulesBase::ExclusionRulesBase(
    const AdEventList& ad_events,
    const SubdivisionTargeting& subdivision_targeting,
    const resource::AntiTargeting& anti_targeting_resource,
    const BrowsingHistoryList& browsing_history) {
  anti_targeting_exclusion_rule_ = std::make_unique<AntiTargetingExclusionRule>(
      anti_targeting_resource, browsing_history);
  exclusion_rules_.push_back(anti_targeting_exclusion_rule_.get());

  conversion_exclusion_rule_ =
      std::make_unique<ConversionExclusionRule>(ad_events);
  exclusion_rules_.push_back(conversion_exclusion_rule_.get());

  daily_cap_exclusion_rule_ =
      std::make_unique<DailyCapExclusionRule>(ad_events);
  exclusion_rules_.push_back(daily_cap_exclusion_rule_.get());

  daypart_exclusion_rule_ = std::make_unique<DaypartExclusionRule>();
  exclusion_rules_.push_back(daypart_exclusion_rule_.get());

  dislike_category_exclusion_rule_ =
      std::make_unique<DislikeCategoryExclusionRule>();
  exclusion_rules_.push_back(dislike_category_exclusion_rule_.get());

  dislike_exclusion_rule_ = std::make_unique<DislikeExclusionRule>();
  exclusion_rules_.push_back(dislike_exclusion_rule_.get());

  marked_as_inappropriate_exclusion_rule_ =
      std::make_unique<MarkedAsInappropriateExclusionRule>();
  exclusion_rules_.push_back(marked_as_inappropriate_exclusion_rule_.get());

  per_day_exclusion_rule_ = std::make_unique<PerDayExclusionRule>(ad_events);
  exclusion_rules_.push_back(per_day_exclusion_rule_.get());

  per_month_exclusion_rule_ =
      std::make_unique<PerMonthExclusionRule>(ad_events);
  exclusion_rules_.push_back(per_month_exclusion_rule_.get());

  per_week_exclusion_rule_ = std::make_unique<PerWeekExclusionRule>(ad_events);
  exclusion_rules_.push_back(per_week_exclusion_rule_.get());

  split_test_exclusion_rule_ = std::make_unique<SplitTestExclusionRule>();
  exclusion_rules_.push_back(split_test_exclusion_rule_.get());

  subdivision_targeting_exclusion_rule_ =
      std::make_unique<SubdivisionTargetingExclusionRule>(
          subdivision_targeting);
  exclusion_rules_.push_back(subdivision_targeting_exclusion_rule_.get());

  total_max_exclusion_rule_ =
      std::make_unique<TotalMaxExclusionRule>(ad_events);
  exclusion_rules_.push_back(total_max_exclusion_rule_.get());

  transferred_exclusion_rule_ =
      std::make_unique<TransferredExclusionRule>(ad_events);
  exclusion_rules_.push_back(transferred_exclusion_rule_.get());
}

ExclusionRulesBase::~ExclusionRulesBase() = default;

bool ExclusionRulesBase::ShouldExcludeCreativeAd(
    const CreativeAdInfo& creative_ad) {
  return base::ranges::any_of(
      exclusion_rules_,
      [=](ExclusionRuleInterface<CreativeAdInfo>* exclusion_rule) {
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

  const auto result = exclusion_rule->ShouldInclude(creative_ad);
  if (result.has_value()) {
    return false;
  }

  BLOG(2, result.error());

  AddToCache(exclusion_rule->GetUuid(creative_ad));

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool ExclusionRulesBase::IsCached(const CreativeAdInfo& creative_ad) const {
  return base::ranges::any_of(uuids_, [&creative_ad](const std::string& uuid) {
    return creative_ad.creative_instance_id == uuid ||
           creative_ad.creative_set_id == uuid ||
           creative_ad.campaign_id == uuid ||
           creative_ad.advertiser_id == uuid || creative_ad.segment == uuid;
  });
}

void ExclusionRulesBase::AddToCache(const std::string& uuid) {
  uuids_.insert(uuid);
}

}  // namespace brave_ads
