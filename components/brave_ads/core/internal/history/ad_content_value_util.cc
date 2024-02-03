/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/history/ad_content_value_util.h"

#include "base/values.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"
#include "brave/components/brave_ads/core/public/history/ad_content_info.h"

namespace brave_ads {

namespace {

constexpr char kType[] = "adType";
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
constexpr char kUserReactionType[] = "likeAction";
constexpr char kConfirmationType[] = "adAction";
constexpr char kIsSaved[] = "savedAd";
constexpr char kIsFlagged[] = "flaggedAd";

constexpr char kLegacyType[] = "type";
constexpr char kLegacyPlacementId[] = "uuid";
constexpr char kLegacyCreativeInstanceId[] = "creative_instance_id";
constexpr char kLegacyCreativeSetId[] = "creative_set_id";
constexpr char kLegacyCampaignId[] = "campaign_id";
constexpr char kLegacyAdvertiserId[] = "advertiser_id";
constexpr char kLegacyBrandInfo[] = "brand_info";
constexpr char kLegacyBrandDisplayUrl[] = "brand_display_url";
constexpr char kLegacyBrandUrl[] = "brand_url";
constexpr char kLegacyUserReactionType[] = "like_action";
constexpr char kLegacyConfirmationType[] = "ad_action";
constexpr char kLegacyIsSaved[] = "saved_ad";
constexpr char kLegacyIsFlagged[] = "flagged_ad";

}  // namespace

base::Value::Dict AdContentToValue(const AdContentInfo& ad_content) {
  return base::Value::Dict()
      .Set(kType, ToString(ad_content.type))
      .Set(kPlacementId, ad_content.placement_id)
      .Set(kCreativeInstanceId, ad_content.creative_instance_id)
      .Set(kCreativeSetId, ad_content.creative_set_id)
      .Set(kCampaignId, ad_content.campaign_id)
      .Set(kAdvertiserId, ad_content.advertiser_id)
      .Set(kSegment, ad_content.segment)
      .Set(kBrand, ad_content.brand)
      .Set(kBrandInfo, ad_content.brand_info)
      .Set(kBrandDisplayUrl, ad_content.brand_display_url)
      .Set(kBrandUrl, ad_content.brand_url.spec())
      .Set(kUserReactionType, static_cast<int>(ad_content.user_reaction_type))
      .Set(kConfirmationType, ToString(ad_content.confirmation_type))
      .Set(kIsSaved, ad_content.is_saved)
      .Set(kIsFlagged, ad_content.is_flagged);
}

// TODO(https://github.com/brave/brave-browser/issues/24934): Reduce cognitive
// complexity.
AdContentInfo AdContentFromValue(const base::Value::Dict& dict) {
  AdContentInfo ad_content;

  if (const auto* const type = dict.FindString(kType)) {
    ad_content.type = ToAdType(*type);
  } else if (const auto* const legacy_ad_type = dict.FindString(kLegacyType)) {
    ad_content.type = ToAdType(*legacy_ad_type);
  } else {
    ad_content.type = AdType::kNotificationAd;
  }

  if (const auto* const placement_id = dict.FindString(kPlacementId)) {
    ad_content.placement_id = *placement_id;
  } else if (const auto* const legacy_placement_id =
                 dict.FindString(kLegacyPlacementId)) {
    ad_content.placement_id = *legacy_placement_id;
  }

  if (const auto* const creative_instance_id =
          dict.FindString(kCreativeInstanceId)) {
    ad_content.creative_instance_id = *creative_instance_id;
  } else if (const auto* const legacy_creative_instance_id =
                 dict.FindString(kLegacyCreativeInstanceId)) {
    ad_content.creative_instance_id = *legacy_creative_instance_id;
  }

  if (const auto* const creative_set_id = dict.FindString(kCreativeSetId)) {
    ad_content.creative_set_id = *creative_set_id;
  } else if (const auto* const legacy_creative_set_id =
                 dict.FindString(kLegacyCreativeSetId)) {
    ad_content.creative_set_id = *legacy_creative_set_id;
  }

  if (const auto* const campaign_id = dict.FindString(kCampaignId)) {
    ad_content.campaign_id = *campaign_id;
  } else if (const auto* const legacy_campaign_id =
                 dict.FindString(kLegacyCampaignId)) {
    ad_content.campaign_id = *legacy_campaign_id;
  }

  if (const auto* const advertiser_id = dict.FindString(kAdvertiserId)) {
    ad_content.advertiser_id = *advertiser_id;
  } else if (const auto* const legacy_advertiser_id =
                 dict.FindString(kLegacyAdvertiserId)) {
    ad_content.advertiser_id = *legacy_advertiser_id;
  }

  if (const auto* const segment = dict.FindString(kSegment)) {
    ad_content.segment = *segment;
  }

  if (const auto* const brand = dict.FindString(kBrand)) {
    ad_content.brand = *brand;
  }

  if (const auto* const brand_info = dict.FindString(kBrandInfo)) {
    ad_content.brand_info = *brand_info;
  } else if (const auto* legacy_brand_info =
                 dict.FindString(kLegacyBrandInfo)) {
    ad_content.brand_info = *legacy_brand_info;
  }

  if (const auto* const brand_display_url = dict.FindString(kBrandDisplayUrl)) {
    ad_content.brand_display_url = *brand_display_url;
  } else if (const auto* legacy_brand_display_url =
                 dict.FindString(kLegacyBrandDisplayUrl)) {
    ad_content.brand_display_url = *legacy_brand_display_url;
  }

  if (const auto* const brand_url = dict.FindString(kBrandUrl)) {
    ad_content.brand_url = GURL(*brand_url);
  } else if (const auto* legacy_brand_url = dict.FindString(kLegacyBrandUrl)) {
    ad_content.brand_url = GURL(*legacy_brand_url);
  }

  if (const auto user_reaction_type = dict.FindInt(kUserReactionType)) {
    ad_content.user_reaction_type =
        static_cast<mojom::UserReactionType>(*user_reaction_type);
  } else if (const auto legacy_user_reaction_type =
                 dict.FindInt(kLegacyUserReactionType)) {
    ad_content.user_reaction_type =
        static_cast<mojom::UserReactionType>(*legacy_user_reaction_type);
  }

  if (const auto* const confirmation_type =
          dict.FindString(kConfirmationType)) {
    ad_content.confirmation_type = ToConfirmationType(*confirmation_type);
  } else if (const auto* const legacy_confirmation_type =
                 dict.FindString(kLegacyConfirmationType)) {
    ad_content.confirmation_type =
        ToConfirmationType(*legacy_confirmation_type);
  }

  if (const auto is_saved = dict.FindBool(kIsSaved)) {
    ad_content.is_saved = *is_saved;
  } else if (const auto legacy_is_saved = dict.FindBool(kLegacyIsSaved)) {
    ad_content.is_saved = *legacy_is_saved;
  }

  if (const auto is_flagged = dict.FindBool(kIsFlagged)) {
    ad_content.is_flagged = *is_flagged;
  } else if (const auto legacy_is_flagged = dict.FindBool(kLegacyIsFlagged)) {
    ad_content.is_flagged = *legacy_is_flagged;
  }

  return ad_content;
}

}  // namespace brave_ads
