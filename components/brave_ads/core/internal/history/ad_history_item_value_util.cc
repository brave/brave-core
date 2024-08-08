/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/history/ad_history_item_value_util.h"

#include "base/json/values_util.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ads_core/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_value_util_internal.h"
#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

namespace brave_ads {

namespace {

// Created at.
constexpr char kCreatedAtKey[] = "createdAt";

// Ad content.
constexpr char kAdContentKey[] = "adContent";
constexpr char kType[] = "adType";
constexpr char kConfirmationType[] = "adAction";
constexpr char kPlacementId[] = "placementId";
constexpr char kCreativeInstanceId[] = "creativeInstanceId";
constexpr char kCreativeSetId[] = "creativeSetId";
constexpr char kCampaignId[] = "campaignId";
constexpr char kAdvertiserId[] = "advertiserId";
constexpr char kSegment[] = "segment";
constexpr char kTitle[] = "brand";
constexpr char kDescription[] = "brandInfo";
constexpr char kTargetUrl[] = "brandUrl";
constexpr char kTargetDisplayUrl[] = "brandDisplayUrl";
constexpr char kLikeAdReactionType[] = "likeAction";
constexpr char kIsSaved[] = "savedAd";
constexpr char kIsAdMarkedAsInappropriate[] = "flaggedAd";

// Segment content.
constexpr char kSegmentContentKey[] = "categoryContent";
constexpr char kSegmentKey[] = "category";
constexpr char kLikeSegmentReactionTypeKey[] = "optAction";

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
