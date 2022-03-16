/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ad_content_info.h"

#include "base/values.h"
#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/logging.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

AdContentInfo::AdContentInfo() = default;

AdContentInfo::AdContentInfo(const AdContentInfo& info) = default;

AdContentInfo::~AdContentInfo() = default;

bool AdContentInfo::operator==(const AdContentInfo& rhs) const {
  return type == rhs.type && uuid == rhs.uuid &&
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

base::DictionaryValue AdContentInfo::ToValue() const {
  base::DictionaryValue dictionary;

  dictionary.SetIntKey("adType", static_cast<int>(type.value()));
  dictionary.SetStringKey("uuid", uuid);
  dictionary.SetStringKey("creativeInstanceId", creative_instance_id);
  dictionary.SetStringKey("creativeSetId", creative_set_id);
  dictionary.SetStringKey("campaignId", campaign_id);
  dictionary.SetStringKey("advertiserId", advertiser_id);
  dictionary.SetStringKey("brand", brand);
  dictionary.SetStringKey("brandInfo", brand_info);
  dictionary.SetStringKey("brandDisplayUrl", brand_display_url);
  dictionary.SetStringKey("brandUrl", brand_url);
  dictionary.SetIntKey("likeAction", static_cast<int>(like_action_type));
  dictionary.SetStringKey("adAction", std::string(confirmation_type));
  dictionary.SetBoolKey("savedAd", is_saved);
  dictionary.SetBoolKey("flaggedAd", is_flagged);

  return dictionary;
}

bool AdContentInfo::FromValue(const base::Value& value) {
  const base::DictionaryValue* dictionary = nullptr;
  if (!(&value)->GetAsDictionary(&dictionary)) {
    return false;
  }

  const absl::optional<int> type_optional = dictionary->FindIntKey("adType");
  if (type_optional) {
    type = AdType(static_cast<AdType::Value>(type_optional.value()));
  }

  const std::string* uuid_value = dictionary->FindStringKey("uuid");
  if (uuid_value) {
    uuid = *uuid_value;
  }

  const std::string* creative_instance_id_value =
      dictionary->FindStringKey("creativeInstanceId");
  if (creative_instance_id_value) {
    creative_instance_id = *creative_instance_id_value;
  }

  const std::string* creative_set_id_value =
      dictionary->FindStringKey("creativeSetId");
  if (creative_set_id_value) {
    creative_set_id = *creative_set_id_value;
  }

  const std::string* campaign_id_value =
      dictionary->FindStringKey("campaignId");
  if (campaign_id_value) {
    campaign_id = *campaign_id_value;
  }

  const std::string* advertiser_id_value =
      dictionary->FindStringKey("advertiserId");
  if (advertiser_id_value) {
    advertiser_id = *advertiser_id_value;
  }

  const std::string* brand_value = dictionary->FindStringKey("brand");
  if (brand_value) {
    brand = *brand_value;
  }

  const std::string* brand_info_value = dictionary->FindStringKey("brandInfo");
  if (brand_info_value) {
    brand_info = *brand_info_value;
  }

  const std::string* brand_display_url_value =
      dictionary->FindStringKey("brandDisplayUrl");
  if (brand_display_url_value) {
    brand_display_url = *brand_display_url_value;
  }

  const std::string* brand_url_value = dictionary->FindStringKey("brandUrl");
  if (brand_url_value) {
    brand_url = *brand_url_value;
  }

  const absl::optional<int> like_action_type_optional =
      dictionary->FindIntKey("likeAction");
  if (like_action_type_optional) {
    like_action_type =
        static_cast<AdContentLikeActionType>(like_action_type_optional.value());
  }

  const std::string* confirmation_type_value =
      dictionary->FindStringKey("adAction");
  if (confirmation_type_value) {
    confirmation_type = ConfirmationType(*confirmation_type_value);
  }

  const absl::optional<bool> is_saved_optional =
      dictionary->FindBoolKey("savedAd");
  if (is_saved_optional) {
    is_saved = is_saved_optional.value();
  }

  const absl::optional<bool> is_flagged_optional =
      dictionary->FindBoolKey("flaggedAd");
  if (is_flagged_optional) {
    is_flagged = is_flagged_optional.value();
  }

  return true;
}

std::string AdContentInfo::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

bool AdContentInfo::FromJson(const std::string& json) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  if (document.HasParseError()) {
    BLOG(1, helper::JSON::GetLastError(&document));
    return false;
  }

  if (document.HasMember("type") && document["type"].IsString()) {
    type = AdType(document["type"].GetString());
  } else {
    type = AdType::kAdNotification;
  }

  if (document.HasMember("uuid")) {
    uuid = document["uuid"].GetString();
  }

  if (document.HasMember("creative_instance_id")) {
    creative_instance_id = document["creative_instance_id"].GetString();
  }

  if (document.HasMember("creative_set_id")) {
    creative_set_id = document["creative_set_id"].GetString();
  }

  if (document.HasMember("campaign_id")) {
    campaign_id = document["campaign_id"].GetString();
  }

  if (document.HasMember("advertiser_id")) {
    advertiser_id = document["advertiser_id"].GetString();
  }

  if (document.HasMember("brand")) {
    brand = document["brand"].GetString();
  }

  if (document.HasMember("brand_info")) {
    brand_info = document["brand_info"].GetString();
  }

  if (document.HasMember("brand_display_url")) {
    brand_display_url = document["brand_display_url"].GetString();
  }

  if (document.HasMember("brand_url")) {
    brand_url = document["brand_url"].GetString();
  }

  if (document.HasMember("like_action")) {
    like_action_type =
        static_cast<AdContentLikeActionType>(document["like_action"].GetInt());
  }

  if (document.HasMember("ad_action")) {
    confirmation_type = ConfirmationType(document["ad_action"].GetString());
  }

  if (document.HasMember("saved_ad")) {
    is_saved = document["saved_ad"].GetBool();
  }

  if (document.HasMember("flagged_ad")) {
    is_flagged = document["flagged_ad"].GetBool();
  }

  return true;
}

void SaveToJson(JsonWriter* writer, const AdContentInfo& info) {
  writer->StartObject();

  writer->String("type");
  const std::string type = std::string(info.type);
  writer->String(type.c_str());

  writer->String("uuid");
  writer->String(info.uuid.c_str());

  writer->String("creative_instance_id");
  writer->String(info.creative_instance_id.c_str());

  writer->String("creative_set_id");
  writer->String(info.creative_set_id.c_str());

  writer->String("campaign_id");
  writer->String(info.campaign_id.c_str());

  writer->String("advertiser_id");
  writer->String(info.advertiser_id.c_str());

  writer->String("brand");
  writer->String(info.brand.c_str());

  writer->String("brand_info");
  writer->String(info.brand_info.c_str());

  writer->String("brand_display_url");
  writer->String(info.brand_display_url.c_str());

  writer->String("brand_url");
  writer->String(info.brand_url.c_str());

  writer->String("like_action");
  writer->Int(static_cast<int>(info.like_action_type));

  writer->String("ad_action");
  const std::string confirmation_type = std::string(info.confirmation_type);
  writer->String(confirmation_type.c_str());

  writer->String("saved_ad");
  writer->Bool(info.is_saved);

  writer->String("flagged_ad");
  writer->Bool(info.is_flagged);

  writer->EndObject();
}

}  // namespace ads
