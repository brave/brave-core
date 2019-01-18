/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/client_info.h"

#include "json_helper.h"

namespace ads {

ClientInfo::ClientInfo() :
    application_version(""),
    platform(ClientInfoPlatformType::UNKNOWN),
    platform_version("") {}

ClientInfo::ClientInfo(const ClientInfo& info) :
    application_version(info.application_version),
    platform(info.platform),
    platform_version(info.platform_version) {}

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

  if (document.HasMember("application_version")) {
    application_version = document["application_version"].GetString();
  }

  if (document.HasMember("platform")) {
    platform = static_cast<ClientInfoPlatformType>
      (document["platform"].GetInt());
  }

  if (document.HasMember("platform_version")) {
    platform_version = document["platform_version"].GetString();
  }

  return SUCCESS;
}

void SaveToJson(JsonWriter* writer, const ClientInfo& info) {
  writer->StartObject();

  writer->String("application_version");
  writer->String(info.application_version.c_str());

  writer->String("platform");
  writer->Int(info.platform);

  writer->String("platform_version");
  writer->String(info.platform_version.c_str());

  writer->EndObject();
}

const std::string ClientInfo::GetPlatformName() const {
  std::string platform_name = "";

  switch (platform) {
    case UNKNOWN: {
      break;
    }
    case WIN7: {
      platform_name = "Win7";
      break;
    }
    case WIN8: {
      platform_name = "Win8";
      break;
    }
    case WIN10: {
      platform_name = "Win10";
      break;
    }
    case MACOS: {
      platform_name = "macOS";
      break;
    }
    case IOS: {
      platform_name = "iOS";
      break;
    }
    case ANDROID_OS: {
      platform_name = "Android";
      break;
    }
    case LINUX: {
      platform_name = "Linux";
      break;
    }
  }

  return platform_name;
}

}  // namespace ads
