/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ad_content_info.h"

#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/logging.h"

namespace ads {

AdContentInfo::AdContentInfo() = default;
AdContentInfo::AdContentInfo(const AdContentInfo& info) = default;
AdContentInfo::~AdContentInfo() = default;

bool AdContentInfo::operator==(const AdContentInfo& rhs) const {
  return type == rhs.type && uuid == rhs.uuid &&
         creative_instance_id == rhs.creative_instance_id &&
         creative_set_id == rhs.creative_set_id &&
         campaign_id == rhs.campaign_id && brand == rhs.brand &&
         brand_info == rhs.brand_info &&
         brand_display_url == rhs.brand_display_url &&
         brand_url == rhs.brand_url && like_action == rhs.like_action &&
         ad_action == rhs.ad_action && saved_ad == rhs.saved_ad &&
         flagged_ad == rhs.flagged_ad;
}

bool AdContentInfo::operator!=(const AdContentInfo& rhs) const {
  return !(*this == rhs);
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
    like_action = static_cast<LikeAction>(document["like_action"].GetInt());
  }

  if (document.HasMember("ad_action")) {
    std::string action = document["ad_action"].GetString();
    ad_action = ConfirmationType(action);
  }

  if (document.HasMember("saved_ad")) {
    saved_ad = document["saved_ad"].GetBool();
  }

  if (document.HasMember("flagged_ad")) {
    flagged_ad = document["flagged_ad"].GetBool();
  }

  return true;
}

void SaveToJson(JsonWriter* writer, const AdContentInfo& ad_content) {
  writer->StartObject();

  writer->String("type");
  auto type = std::string(ad_content.type);
  writer->String(type.c_str());

  writer->String("uuid");
  writer->String(ad_content.uuid.c_str());

  writer->String("creative_instance_id");
  writer->String(ad_content.creative_instance_id.c_str());

  writer->String("creative_set_id");
  writer->String(ad_content.creative_set_id.c_str());

  writer->String("campaign_id");
  writer->String(ad_content.campaign_id.c_str());

  writer->String("brand");
  writer->String(ad_content.brand.c_str());

  writer->String("brand_info");
  writer->String(ad_content.brand_info.c_str());

  writer->String("brand_display_url");
  writer->String(ad_content.brand_display_url.c_str());

  writer->String("brand_url");
  writer->String(ad_content.brand_url.c_str());

  writer->String("like_action");
  writer->Int(static_cast<int>(ad_content.like_action));

  writer->String("ad_action");
  auto ad_action = std::string(ad_content.ad_action);
  writer->String(ad_action.c_str());

  writer->String("saved_ad");
  writer->Bool(ad_content.saved_ad);

  writer->String("flagged_ad");
  writer->Bool(ad_content.flagged_ad);

  writer->EndObject();
}

}  // namespace ads
