/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/client_info.h"

#include "bat/ads/internal/json_helper.h"

namespace ads {

ClientInfo::ClientInfo() = default;

ClientInfo::ClientInfo(
    const ClientInfo& info) = default;

ClientInfo::~ClientInfo() = default;

const std::string ClientInfo::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

Result ClientInfo::FromJson(
    const std::string& json,
    std::string* error_description) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  if (document.HasParseError()) {
    if (error_description) {
      *error_description = helper::JSON::GetLastError(&document);
    }

    return FAILED;
  }

  if (document.HasMember("platform")) {
    platform = static_cast<ClientInfoPlatformType>
        (document["platform"].GetInt());
  }

  return SUCCESS;
}

void SaveToJson(JsonWriter* writer, const ClientInfo& info) {
  writer->StartObject();

  writer->String("platform");
  writer->Int(info.platform);

  writer->EndObject();
}

}  // namespace ads
