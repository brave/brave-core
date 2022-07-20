/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ad_content_info.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "bat/ads/internal/base/logging_util.h"

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
  } else {
    return AdContentLikeActionType::kThumbsUp;
  }
}

AdContentLikeActionType AdContentInfo::ToggleThumbDownActionType() const {
  if (like_action_type == AdContentLikeActionType::kThumbsDown) {
    return AdContentLikeActionType::kNeutral;
  } else {
    return AdContentLikeActionType::kThumbsDown;
  }
}

base::Value::Dict AdContentInfo::ToValue() const {
  base::Value::Dict dictionary;

  dictionary.Set("adType", static_cast<int>(type.value()));
  dictionary.Set("uuid", placement_id);
  dictionary.Set("creativeInstanceId", creative_instance_id);
  dictionary.Set("creativeSetId", creative_set_id);
  dictionary.Set("campaignId", campaign_id);
  dictionary.Set("advertiserId", advertiser_id);
  dictionary.Set("brand", brand);
  dictionary.Set("brandInfo", brand_info);
  dictionary.Set("brandDisplayUrl", brand_display_url);
  dictionary.Set("brandUrl", brand_url.spec());
  dictionary.Set("likeAction", static_cast<int>(like_action_type));
  dictionary.Set("adAction", confirmation_type.ToString());
  dictionary.Set("savedAd", is_saved);
  dictionary.Set("flaggedAd", is_flagged);

  return dictionary;
}

bool AdContentInfo::FromValue(const base::Value::Dict& root) {
  if (const auto* value = root.FindString("type")) {
    type = AdType(*value);
  } else {
    type = AdType::kNotificationAd;
  }

  if (const auto* value = root.FindString("uuid")) {
    placement_id = *value;
  }

  if (const auto* value = root.FindString("creative_instance_id")) {
    creative_instance_id = *value;
  }

  if (const auto* value = root.FindString("creative_set_id")) {
    creative_set_id = *value;
  }

  if (const auto* value = root.FindString("campaign_id")) {
    campaign_id = *value;
  }

  if (const auto* value = root.FindString("advertiser_id")) {
    advertiser_id = *value;
  }

  if (const auto* value = root.FindString("brand")) {
    brand = *value;
  }

  if (const auto* value = root.FindString("brand_info")) {
    brand_info = *value;
  }

  if (const auto* value = root.FindString("brand_display_url")) {
    brand_display_url = *value;
  }

  if (const auto* value = root.FindString("brand_url")) {
    brand_url = GURL(*value);
  }

  if (const auto value = root.FindInt("like_action")) {
    like_action_type = static_cast<AdContentLikeActionType>(*value);
  }

  if (const auto* value = root.FindString("ad_action")) {
    confirmation_type = ConfirmationType(*value);
  }

  if (const auto value = root.FindBool("saved_ad")) {
    is_saved = *value;
  }

  if (const auto value = root.FindBool("flagged_ad")) {
    is_flagged = *value;
  }

  return true;
}

std::string AdContentInfo::ToJson() const {
  std::string json;
  base::JSONWriter::Write(ToValue(), &json);
  return json;
}

bool AdContentInfo::FromJson(const std::string& json) {
  absl::optional<base::Value> document =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);

  if (!document.has_value() || !document->is_dict()) {
    return false;
  }

  return FromValue(document->GetDict());
}

}  // namespace ads
