/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/publisher_ad_info.h"
#include "bat/ads/confirmation_type.h"

#include "bat/ads/internal/json_helper.h"

#include "base/logging.h"

namespace ads {

PublisherAdInfo::PublisherAdInfo() = default;

PublisherAdInfo::PublisherAdInfo(
    const PublisherAdInfo& info) = default;

PublisherAdInfo::~PublisherAdInfo() = default;

const std::string PublisherAdInfo::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

Result PublisherAdInfo::FromJson(
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

  if (document.HasMember("creative_set_id")) {
    creative_set_id = document["creative_set_id"].GetString();
  }

  if (document.HasMember("category")) {
    category = document["category"].GetString();
  }

  if (document.HasMember("size")) {
    size = document["size"].GetString();
  }

  if (document.HasMember("creative_url")) {
    creative_url = document["creative_url"].GetString();
  }

  if (document.HasMember("target_url")) {
    target_url = document["target_url"].GetString();
  }

  if (document.HasMember("uuid")) {
    creative_instance_id = document["uuid"].GetString();
  }

  if (document.HasMember("confirmation_type")) {
    confirmation_type =
        ConfirmationType(document["confirmation_type"].GetString());
  }

  return SUCCESS;
}

void SaveToJson(JsonWriter* writer, const PublisherAdInfo& info) {
  DCHECK(writer);
  if (!writer) {
    return;
  }

  writer->StartObject();

  writer->String("creative_set_id");
  writer->String(info.creative_set_id.c_str());

  writer->String("category");
  writer->String(info.category.c_str());

  writer->String("size");
  writer->String(info.size.c_str());

  writer->String("creative_url");
  writer->String(info.creative_url.c_str());

  writer->String("target_url");
  writer->String(info.target_url.c_str());

  writer->String("uuid");
  writer->String(info.creative_instance_id.c_str());

  writer->String("confirmation_type");
  auto type = std::string(info.confirmation_type);
  writer->String(type.c_str());

  writer->EndObject();
}

}  // namespace ads
