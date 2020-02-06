/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/issuers_info.h"

#include "bat/ads/internal/json_helper.h"

namespace ads {

IssuersInfo::IssuersInfo() = default;

IssuersInfo::IssuersInfo(
    const IssuersInfo& info) = default;

IssuersInfo::~IssuersInfo() = default;

const std::string IssuersInfo::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

Result IssuersInfo::FromJson(
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

  // Public key
  if (!document.HasMember("public_key")) {
    if (error_description) {
      *error_description = "Catalog issuers public key is missing";
    }

    return FAILED;
  }

  public_key = document["public_key"].GetString();

  // Issuers
  if (!document.HasMember("issuers")) {
    if (error_description) {
      *error_description = "No catalog issuers";
    }

    return FAILED;
  }

  std::vector<IssuerInfo> new_issuers = {};
  for (const auto& issuer : document["issuers"].GetArray()) {
    IssuerInfo info;
    info.name = issuer["name"].GetString();
    info.public_key = issuer["public_key"].GetString();

    new_issuers.push_back(info);
  }

  issuers = new_issuers;

  return SUCCESS;
}

void SaveToJson(JsonWriter* writer, const IssuersInfo& info) {
  writer->StartObject();

  // Public key
  writer->String("public_key");
  writer->String(info.public_key.c_str());

  // Issuers
  writer->String("issuers");
  writer->StartArray();
  for (const auto& issuer : info.issuers) {
    writer->StartObject();

    writer->String("name");
    writer->String(issuer.name.c_str());

    writer->String("public_key");
    writer->String(issuer.public_key.c_str());

    writer->EndObject();
  }
  writer->EndArray();

  writer->EndObject();
}

}  // namespace ads
