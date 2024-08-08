/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/history/ad_history_value_util.h"

#include "base/json/values_util.h"
#include "base/strings/string_number_conversions.h"
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
constexpr char kBrand[] = "brand";
constexpr char kBrandInfo[] = "brandInfo";
constexpr char kBrandDisplayUrl[] = "brandDisplayUrl";
constexpr char kBrandUrl[] = "brandUrl";
constexpr char kAdReactionType[] = "likeAction";
constexpr char kCategoryReactionTypeKey[] = "optAction";
constexpr char kIsSaved[] = "savedAd";
constexpr char kIsMarkedAsInappropriate[] = "flaggedAd";

// Category content.
constexpr char kCategoryContentKey[] = "categoryContent";
constexpr char kLegacyCategoryContentKey[] = "category_content";
constexpr char kCategoryKey[] = "category";

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

  if (const auto* const brand = content_dict->FindString(kBrand)) {
    ad_history_item.brand = *brand;
  }

  if (const auto* const brand_info = content_dict->FindString(kBrandInfo)) {
    ad_history_item.brand_info = *brand_info;
  }

  if (const auto* const brand_display_url =
          content_dict->FindString(kBrandDisplayUrl)) {
    ad_history_item.brand_display_url = *brand_display_url;
  }

  if (const auto* const brand_url = content_dict->FindString(kBrandUrl)) {
    ad_history_item.brand_url = GURL(*brand_url);
  }

  if (const auto reaction_type = content_dict->FindInt(kAdReactionType)) {
    ad_history_item.ad_reaction_type =
        static_cast<mojom::ReactionType>(*reaction_type);
  }

  if (const auto is_saved = content_dict->FindBool(kIsSaved)) {
    ad_history_item.is_saved = *is_saved;
  }

  if (const auto is_marked_as_inappropriate =
          content_dict->FindBool(kIsMarkedAsInappropriate)) {
    ad_history_item.is_marked_as_inappropriate = *is_marked_as_inappropriate;
  }
}

void ParseCategoryContent(const base::Value::Dict& dict,
                          AdHistoryItemInfo& ad_history_item) {
  const base::Value::Dict* content_dict = nullptr;

  content_dict = dict.FindDict(kCategoryContentKey);
  if (!content_dict) {
    // Migration from legacy key.
    content_dict = dict.FindDict(kLegacyCategoryContentKey);
    if (!content_dict) {
      return;
    }
  }

  if (const auto* const segment = content_dict->FindString(kCategoryKey)) {
    ad_history_item.segment = *segment;
  }

  if (const auto reaction_type =
          content_dict->FindInt(kCategoryReactionTypeKey)) {
    ad_history_item.segment_reaction_type =
        static_cast<mojom::ReactionType>(*reaction_type);
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
               .Set(kBrand, ad_history_item.brand)
               .Set(kBrandInfo, ad_history_item.brand_info)
               .Set(kBrandDisplayUrl, ad_history_item.brand_display_url)
               .Set(kBrandUrl, ad_history_item.brand_url.spec())
               .Set(kAdReactionType,
                    static_cast<int>(ad_history_item.ad_reaction_type))
               .Set(kIsSaved, ad_history_item.is_saved)
               .Set(kIsMarkedAsInappropriate,
                    ad_history_item.is_marked_as_inappropriate))
      .Set(kCategoryContentKey,
           base::Value::Dict()
               .Set(kCategoryKey, ad_history_item.segment)
               .Set(kCategoryReactionTypeKey,
                    static_cast<int>(ad_history_item.segment_reaction_type)));
}

AdHistoryItemInfo AdHistoryItemFromValue(const base::Value::Dict& dict) {
  AdHistoryItemInfo ad_history_item;

  ParseCreatedAt(dict, ad_history_item);
  ParseAdContent(dict, ad_history_item);
  ParseCategoryContent(dict, ad_history_item);

  return ad_history_item;
}

base::Value::List AdHistoryToValue(const AdHistoryList& ad_history) {
  base::Value::List list;
  list.reserve(ad_history.size());

  for (const auto& ad_history_item : ad_history) {
    list.Append(AdHistoryItemToValue(ad_history_item));
  }

  return list;
}

AdHistoryList AdHistoryFromValue(const base::Value::List& list) {
  AdHistoryList ad_history;
  ad_history.reserve(list.size());

  for (const auto& value : list) {
    if (const auto* const dict = value.GetIfDict()) {
      ad_history.push_back(AdHistoryItemFromValue(*dict));
    }
  }

  return ad_history;
}

base::Value::List AdHistoryToUIValue(const AdHistoryList& ad_history) {
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
