/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/bundle_state.h"

namespace ads {

BUNDLE_STATE::BUNDLE_STATE() :
    catalog_id(""),
    catalog_version(0),
    catalog_ping(0),
    categories({}) {}

BUNDLE_STATE::BUNDLE_STATE(const BUNDLE_STATE& state) {
    catalog_id = state.catalog_id;
    catalog_version = state.catalog_version;
    catalog_ping = state.catalog_ping;
    categories = state.categories;
}

BUNDLE_STATE::~BUNDLE_STATE() = default;

bool BUNDLE_STATE::LoadFromJson(const std::string& json) {
  rapidjson::Document bundle;
  bundle.Parse(json.c_str());

  if (bundle.HasParseError()) {
    return false;
  }

  const std::map<std::string, std::string> members = {
    {"categories", "Object"}
  };

  // TODO(Terry Mancey): Decouple validateJson by moving to json_helper class
  if (!validateJson(bundle, members)) {
    return false;
  }

  std::map<std::string, std::vector<bundle::CategoryInfo>> new_categories;

  if (bundle.HasMember("categories")) {
    for (const auto& category : bundle["categories"].GetObject()) {
      for (const auto& info : category.value.GetArray()) {
        if (!info.HasMember("advertiser") &&
            !info.HasMember("notificationText") &&
            !info.HasMember("notificationURL") &&
            !info.HasMember("uuid")) {
          continue;
        }

        bundle::CategoryInfo category_info;

        if (info.HasMember("creativeSetId")) {
          category_info.creative_set_id = info["creativeSetId"].GetString();
        }

        category_info.advertiser = info["advertiser"].GetString();
        category_info.notification_text = info["notificationText"].GetString();
        category_info.notification_url = info["notificationURL"].GetString();
        category_info.uuid = info["uuid"].GetString();

        if (new_categories.find(category.name.GetString()) ==
            new_categories.end()) {
          new_categories.insert({category.name.GetString(), {}});
        }
        new_categories.at(category.name.GetString()).push_back(category_info);
      }
    }
  }

  categories = new_categories;

  return true;
}

// TODO(Terry Mancey): Decouple validateJson by moving to json_helper class
bool BUNDLE_STATE::validateJson(
    const rapidjson::Document& document,
    const std::map<std::string, std::string>& members) {
  for (const auto& member : document.GetObject()) {
    std::string member_name = member.name.GetString();
    std::string member_type = _rapidjson_member_types[member.value.GetType()];

    if (members.find(member_name) == members.end()) {
      // Member name not used
      continue;
    }

    std::string type = members.at(member_name);
    if (type != member_type) {
      // Invalid member type
      return false;
    }
  }

  return true;
}

void SaveToJson(JsonWriter& writer, const BUNDLE_STATE& state) {
  writer.StartObject();

  writer.String("categories");
  writer.StartObject();

  for (const auto& category : state.categories) {
    writer.String(category.first.c_str());
    writer.StartArray();

    for (const auto& category : category.second) {
      writer.StartObject();

      writer.String("advertiser");
      writer.String(category.advertiser.c_str());

      writer.String("notificationText");
      writer.String(category.notification_text.c_str());

      writer.String("notificationURL");
      writer.String(category.notification_url.c_str());

      writer.String("uuid");
      writer.String(category.uuid.c_str());

      writer.String("creativeSetId");
      writer.String(category.creative_set_id.c_str());

      writer.EndObject();
    }

    writer.EndArray();
  }

  writer.EndObject();

  writer.EndObject();
}

}  // namespace ads
