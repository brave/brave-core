/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ad_content.h"
#include "bat/ads/confirmation_type.h"

#include "bat/ads/internal/json_helper.h"

#include "base/logging.h"

namespace ads {

AdContent::AdContent() = default;

AdContent::AdContent(
    const AdContent& properties) = default;

AdContent::~AdContent() = default;

bool AdContent::operator==(
    const AdContent& rhs) const {
  return uuid == rhs.uuid &&
      creative_set_id == rhs.creative_set_id &&
      brand == rhs.brand &&
      brand_info == rhs.brand_info &&
      brand_logo == rhs.brand_logo &&
      brand_display_url == rhs.brand_display_url &&
      brand_url == rhs.brand_url &&
      like_action == rhs.like_action &&
      ad_action == rhs.ad_action &&
      saved_ad == rhs.saved_ad &&
      flagged_ad == rhs.flagged_ad;
}

bool AdContent::operator!=(
    const AdContent& rhs) const {
  return !(*this == rhs);
}

const std::string AdContent::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

Result AdContent::FromJson(
    const std::string& json,
    std::string* error_description) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  if (document.HasParseError()) {
    if (error_description != nullptr) {
      *error_description = helper::JSON::GetLastError(&document);
    }

    return FAILED;
  }

  if (document.HasMember("uuid")) {
    uuid = document["uuid"].GetString();
  }

  if (document.HasMember("creative_set_id")) {
    creative_set_id = document["creative_set_id"].GetString();
  }

  if (document.HasMember("brand")) {
    brand = document["brand"].GetString();
  }

  if (document.HasMember("brand_info")) {
    brand_info = document["brand_info"].GetString();
  }

  if (document.HasMember("brand_logo")) {
    brand_logo = document["brand_logo"].GetString();
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

  return SUCCESS;
}

void SaveToJson(JsonWriter* writer, const AdContent& content) {
  writer->StartObject();

  writer->String("uuid");
  writer->String(content.uuid.c_str());

  writer->String("creative_set_id");
  writer->String(content.creative_set_id.c_str());

  writer->String("brand");
  writer->String(content.brand.c_str());

  writer->String("brand_info");
  writer->String(content.brand_info.c_str());

  writer->String("brand_logo");
  writer->String(content.brand_logo.c_str());

  writer->String("brand_display_url");
  writer->String(content.brand_display_url.c_str());

  writer->String("brand_url");
  writer->String(content.brand_url.c_str());

  writer->String("like_action");
  writer->Int(content.like_action);

  writer->String("ad_action");
  auto ad_action = std::string(content.ad_action);
  writer->String(ad_action.c_str());

  writer->String("saved_ad");
  writer->Bool(content.saved_ad);

  writer->String("flagged_ad");
  writer->Bool(content.flagged_ad);

  writer->EndObject();
}

}  // namespace ads
