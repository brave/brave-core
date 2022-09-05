/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ad_content_info.h"

#include "absl/types/optional.h"
#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"

namespace ads {

AdContentInfo::AdContentInfo() = default;

AdContentInfo::AdContentInfo(const AdContentInfo& info) = default;

AdContentInfo& AdContentInfo::operator=(const AdContentInfo& info) = default;

AdContentInfo::~AdContentInfo() = default;

bool AdContentInfo::operator==(const AdContentInfo& rhs) const {
  return type == rhs.type && placement_id == rhs.placement_id &&
         creative_instance_id == rhs.creative_instance_id &&
         creative_set_id == rhs.creative_set_id &&
         campaign_id == rhs.campaign_id && advertiser_id == rhs.advertiser_id &&
         brand == rhs.brand && brand_info == rhs.brand_info &&
         brand_display_url == rhs.brand_display_url &&
         brand_url == rhs.brand_url &&
         like_action_type == rhs.like_action_type &&
         confirmation_type == rhs.confirmation_type &&
         is_saved == rhs.is_saved && is_flagged == rhs.is_flagged;
}

bool AdContentInfo::operator!=(const AdContentInfo& rhs) const {
  return !(*this == rhs);
}

AdContentLikeActionType AdContentInfo::ToggleThumbUpActionType() const {
  if (like_action_type == AdContentLikeActionType::kThumbsUp) {
    return AdContentLikeActionType::kNeutral;
  }

  return AdContentLikeActionType::kThumbsUp;
}

AdContentLikeActionType AdContentInfo::ToggleThumbDownActionType() const {
  if (like_action_type == AdContentLikeActionType::kThumbsDown) {
    return AdContentLikeActionType::kNeutral;
  }

  return AdContentLikeActionType::kThumbsDown;
}

base::Value::Dict AdContentInfo::ToValue() const {
  base::Value::Dict dict;

  dict.Set("adType", int{type.value()});
  dict.Set("uuid", placement_id);
  dict.Set("creativeInstanceId", creative_instance_id);
  dict.Set("creativeSetId", creative_set_id);
  dict.Set("campaignId", campaign_id);
  dict.Set("advertiserId", advertiser_id);
  dict.Set("brand", brand);
  dict.Set("brandInfo", brand_info);
  dict.Set("brandDisplayUrl", brand_display_url);
  dict.Set("brandUrl", brand_url.spec());
  dict.Set("likeAction", static_cast<int>(like_action_type));
  dict.Set("adAction", confirmation_type.ToString());
  dict.Set("savedAd", is_saved);
  dict.Set("flaggedAd", is_flagged);

  return dict;
}

void AdContentInfo::FromValue(const base::Value::Dict& root) {
  if (const auto* value = root.FindString("adType")) {
    type = AdType(*value);
  } else if (const auto* value = root.FindString("type")) {
    // Migrate legacy
    type = AdType(*value);
  } else {
    type = AdType::kNotificationAd;
  }

  if (const auto* value = root.FindString("uuid")) {
    placement_id = *value;
  }

  if (const auto* value = root.FindString("creativeInstanceId")) {
    creative_instance_id = *value;
  } else if (const auto* value = root.FindString("creative_instance_id")) {
    // Migrate legacy
    creative_instance_id = *value;
  }

  if (const auto* value = root.FindString("creativeSetId")) {
    creative_set_id = *value;
  } else if (const auto* value = root.FindString("creative_set_id")) {
    // Migrate legacy
    creative_set_id = *value;
  }

  if (const auto* value = root.FindString("campaignId")) {
    campaign_id = *value;
  } else if (const auto* value = root.FindString("campaign_id")) {
    // Migrate legacy
    campaign_id = *value;
  }

  if (const auto* value = root.FindString("advertiserId")) {
    advertiser_id = *value;
  } else if (const auto* value = root.FindString("advertiser_id")) {
    // Migrate legacy
    advertiser_id = *value;
  }

  if (const auto* value = root.FindString("brand")) {
    brand = *value;
  }

  if (const auto* value = root.FindString("brandInfo")) {
    brand_info = *value;
  } else if (const auto* value = root.FindString("brand_info")) {
    // Migrate legacy
    brand_info = *value;
  }

  if (const auto* value = root.FindString("brandDisplayUrl")) {
    brand_display_url = *value;
  } else if (const auto* value = root.FindString("brand_display_url")) {
    // Migrate legacy
    brand_display_url = *value;
  }

  if (const auto* value = root.FindString("brandUrl")) {
    brand_url = GURL(*value);
  } else if (const auto* value = root.FindString("brand_url")) {
    // Migrate legacy
    brand_url = GURL(*value);
  }

  if (const auto value = root.FindInt("likeAction")) {
    like_action_type = static_cast<AdContentLikeActionType>(*value);
  } else if (const auto value = root.FindInt("like_action")) {
    // Migrate legacy
    like_action_type = static_cast<AdContentLikeActionType>(*value);
  }

  if (const auto* value = root.FindString("adAction")) {
    confirmation_type = ConfirmationType(*value);
  } else if (const auto* value = root.FindString("ad_action")) {
    // Migrate legacy
    confirmation_type = ConfirmationType(*value);
  }

  if (const auto value = root.FindBool("savedAd")) {
    is_saved = *value;
  } else if (const auto value = root.FindBool("saved_ad")) {
    // Migrate legacy
    is_saved = *value;
  }

  if (const auto value = root.FindBool("flaggedAd")) {
    is_flagged = *value;
  } else if (const auto value = root.FindBool("flagged_ad")) {
    // Migrate legacy
    is_flagged = *value;
  }
}

std::string AdContentInfo::ToJson() const {
  std::string json;
  CHECK(base::JSONWriter::Write(ToValue(), &json));
  return json;
}

bool AdContentInfo::FromJson(const std::string& json) {
  const absl::optional<base::Value> root =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);

  if (!root || !root->is_dict()) {
    return false;
  }

  FromValue(root->GetDict());

  return true;
}

}  // namespace ads
