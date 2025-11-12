/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/history/ad_history_item_value_util.h"

#include <string_view>

#include "base/json/values_util.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ads_core/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_value_util_internal.h"
#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

namespace brave_ads {

namespace {

// Created at.
constexpr std::string_view kCreatedAtKey = "createdAt";

// Ad content.
constexpr std::string_view kAdContentKey = "adContent";
constexpr std::string_view kType = "adType";
constexpr std::string_view kConfirmationType = "adAction";
constexpr std::string_view kPlacementId = "placementId";
constexpr std::string_view kCreativeInstanceId = "creativeInstanceId";
constexpr std::string_view kCreativeSetId = "creativeSetId";
constexpr std::string_view kCampaignId = "campaignId";
constexpr std::string_view kAdvertiserId = "advertiserId";
constexpr std::string_view kSegment = "segment";
constexpr std::string_view kTitle = "brand";
constexpr std::string_view kDescription = "brandInfo";
constexpr std::string_view kTargetUrl = "brandUrl";
constexpr std::string_view kTargetDisplayUrl = "brandDisplayUrl";
constexpr std::string_view kLikeAdReactionType = "likeAction";
constexpr std::string_view kIsSaved = "savedAd";
constexpr std::string_view kIsAdMarkedAsInappropriate = "flaggedAd";

// Segment content.
constexpr std::string_view kSegmentContentKey = "categoryContent";
constexpr std::string_view kSegmentKey = "category";
constexpr std::string_view kLikeSegmentReactionTypeKey = "optAction";

}  // namespace

AdHistoryItemInfo AdHistoryItemFromValue(const base::Value::Dict& dict) {
  AdHistoryItemInfo ad_history_item;

  ParseCreatedAt(dict, ad_history_item);
  ParseAdContent(dict, ad_history_item);
  ParseSegmentContent(dict, ad_history_item);

  return ad_history_item;
}

base::Value::Dict AdHistoryItemToValue(
    const AdHistoryItemInfo& ad_history_item) {
  return base::Value::Dict()
      .Set(kCreatedAtKey, base::TimeToValue(ad_history_item.created_at))
      .Set(kAdContentKey,
           base::Value::Dict()
               .Set(kType, ToString(ad_history_item.type))
               .Set(kConfirmationType,
                    ToString(ad_history_item.confirmation_type))
               .Set(kPlacementId, ad_history_item.placement_id)
               .Set(kCreativeInstanceId, ad_history_item.creative_instance_id)
               .Set(kCreativeSetId, ad_history_item.creative_set_id)
               .Set(kCampaignId, ad_history_item.campaign_id)
               .Set(kAdvertiserId, ad_history_item.advertiser_id)
               .Set(kSegment, ad_history_item.segment)
               .Set(kTitle, ad_history_item.title)
               .Set(kDescription, ad_history_item.description)
               .Set(kTargetUrl, ad_history_item.target_url.spec())
               .Set(kTargetDisplayUrl, ad_history_item.target_url.host())
               .Set(kLikeAdReactionType,
                    static_cast<int>(GetReactions().AdReactionTypeForId(
                        ad_history_item.advertiser_id)))
               .Set(kIsSaved, GetReactions().IsAdSaved(
                                  ad_history_item.creative_instance_id))
               .Set(kIsAdMarkedAsInappropriate,
                    GetReactions().IsAdMarkedAsInappropriate(
                        ad_history_item.creative_set_id)))
      .Set(kSegmentContentKey,
           base::Value::Dict()
               .Set(kSegmentKey, ad_history_item.segment)
               .Set(kLikeSegmentReactionTypeKey,
                    static_cast<int>(GetReactions().SegmentReactionTypeForId(
                        ad_history_item.segment))));
}

}  // namespace brave_ads
