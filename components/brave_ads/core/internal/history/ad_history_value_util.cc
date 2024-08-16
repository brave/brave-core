/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/history/ad_history_value_util.h"

#include "base/json/values_util.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_ads/core/internal/ads_core/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

namespace brave_ads {

namespace {

// Created at.
constexpr char kCreatedAtKey[] = "createdAt";
constexpr char kLegacyCreatedAtKey[] = "created_at";

// Ad content.
constexpr char kAdContentKey[] = "adContent";
constexpr char kLegacyAdContentKey[] = "ad_content";
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
constexpr char kLegacySegmentContentKey[] = "category_content";
constexpr char kSegmentKey[] = "category";
constexpr char kLikeSegmentReactionTypeKey[] = "optAction";

// UI
constexpr char kUIUuidKey[] = "uuid";
constexpr char kUICreatedAtKey[] = "timestampInMilliseconds";
constexpr char kUIRowKey[] = "adDetailRows";

void ParseCreatedAt(const base::Value::Dict& dict,
                    AdHistoryItemInfo& ad_history_item) {
  const base::Value* value = dict.Find(kCreatedAtKey);
  if (!value) {
    // Migration from legacy key.
    value = dict.Find(kLegacyCreatedAtKey);
  }

  ad_history_item.created_at = base::ValueToTime(value).value_or(base::Time());
}

void ParseAdContent(const base::Value::Dict& dict,
                    AdHistoryItemInfo& ad_history_item) {
  const base::Value::Dict* content_dict = dict.FindDict(kAdContentKey);
  if (!content_dict) {
    // Migration from legacy key.
    content_dict = dict.FindDict(kLegacyAdContentKey);
    if (!content_dict) {
      return;
    }
  }

  if (const auto* const type = content_dict->FindString(kType)) {
    ad_history_item.type = ToAdType(*type);
  }

  if (const auto* const confirmation_type =
          content_dict->FindString(kConfirmationType)) {
    ad_history_item.confirmation_type = ToConfirmationType(*confirmation_type);
  }

  if (const auto* const placement_id = content_dict->FindString(kPlacementId)) {
    ad_history_item.placement_id = *placement_id;
  }

  if (const auto* const creative_instance_id =
          content_dict->FindString(kCreativeInstanceId)) {
    ad_history_item.creative_instance_id = *creative_instance_id;
  }

  if (const auto* const creative_set_id =
          content_dict->FindString(kCreativeSetId)) {
    ad_history_item.creative_set_id = *creative_set_id;
  }

  if (const auto* const campaign_id = content_dict->FindString(kCampaignId)) {
    ad_history_item.campaign_id = *campaign_id;
  }

  if (const auto* const advertiser_id =
          content_dict->FindString(kAdvertiserId)) {
    ad_history_item.advertiser_id = *advertiser_id;
  }

  if (const auto* const segment = content_dict->FindString(kSegment)) {
    ad_history_item.segment = *segment;
  }

  if (const auto* const title = content_dict->FindString(kTitle)) {
    ad_history_item.title = *title;
  }

  if (const auto* const description = content_dict->FindString(kDescription)) {
    ad_history_item.description = *description;
  }

  if (const auto* const target_url = content_dict->FindString(kTargetUrl)) {
    ad_history_item.target_url = GURL(*target_url);
  }
}

void ParseSegmentContent(const base::Value::Dict& dict,
                         AdHistoryItemInfo& ad_history_item) {
  const base::Value::Dict* content_dict = nullptr;

  content_dict = dict.FindDict(kSegmentContentKey);
  if (!content_dict) {
    // Migration from legacy key.
    content_dict = dict.FindDict(kLegacySegmentContentKey);
    if (!content_dict) {
      return;
    }
  }

  if (const auto* const segment = content_dict->FindString(kSegmentKey)) {
    ad_history_item.segment = *segment;
  }
}

base::Value::List AdHistoryItemToUIValue(
    const AdHistoryItemInfo& ad_history_item) {
  return base::Value::List().Append(AdHistoryItemToValue(ad_history_item));
}

}  // namespace

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

AdHistoryItemInfo AdHistoryItemFromValue(const base::Value::Dict& dict) {
  AdHistoryItemInfo ad_history_item;

  ParseCreatedAt(dict, ad_history_item);
  ParseAdContent(dict, ad_history_item);
  ParseSegmentContent(dict, ad_history_item);

  return ad_history_item;
}

base::Value::List AdHistoryToValue(const AdHistoryList& ad_history) {
  base::Value::List list;
  list.reserve(ad_history.size());

  int row = 0;

  for (const auto& ad_history_item : ad_history) {
    list.Append(base::Value::Dict()
                    .Set(kUIUuidKey, base::NumberToString(row++))
                    .Set(kUICreatedAtKey,
                         ad_history_item.created_at
                             .InMillisecondsFSinceUnixEpochIgnoringNull())
                    .Set(kUIRowKey, AdHistoryItemToUIValue(ad_history_item)));
  }

  return list;
}

}  // namespace brave_ads
