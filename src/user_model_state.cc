/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../include/user_model_state.h"
#include "../include/json_helper.h"
#include "../include/static_values.h"

namespace state {

USER_MODEL_STATE::USER_MODEL_STATE() :
    ads_shown_history({}),
    ad_uuid(""),
    ads_uuid_seen({}),
    available(false),
    allowed(false),
    configured(false),
    current_ssid(""),
    expired(false),
    final_contact_timestamp(std::time(nullptr)),
    first_contact_timestamp(std::time(nullptr)),
    last_search_time(std::time(nullptr)),
    last_shop_time(std::time(nullptr)),
    last_user_activity(std::time(nullptr)),
    last_user_idle_stop_time(std::time(nullptr)),
    locale(""),
    locales({}),
    page_score_history({}),
    places({}),
    score(0.0),
    search_activity(false),
    search_url(""),
    shop_activity(false),
    shop_url(""),
    status("") {}

USER_MODEL_STATE::USER_MODEL_STATE(const USER_MODEL_STATE& state) {
  ads_shown_history = state.ads_shown_history;
  ad_uuid = state.ad_uuid;
  ads_uuid_seen = state.ads_uuid_seen;
  available = state.available;
  allowed = state.allowed;
  configured = state.configured;
  current_ssid = state.current_ssid;
  expired = state.expired;
  final_contact_timestamp = state.final_contact_timestamp;
  first_contact_timestamp = state.first_contact_timestamp;
  last_search_time = state.last_search_time;
  last_shop_time = state.last_shop_time;
  last_user_activity = state.last_user_activity;
  last_user_idle_stop_time = state.last_user_idle_stop_time;
  locale = state.locale;
  locales = state.locales;
  page_score_history = state.page_score_history;
  places = state.places;
  score = state.score;
  search_activity = state.search_activity;
  search_url = state.search_url;
  shop_activity = state.shop_activity;
  shop_url = state.shop_url;
  status = state.status;
}

USER_MODEL_STATE::~USER_MODEL_STATE() = default;

bool USER_MODEL_STATE::LoadFromJson(const std::string& json) {
  rapidjson::Document user_model;
  user_model.Parse(json.c_str());

  if (user_model.HasParseError()) {
    LOG(ERROR) << "Failed to parse User Model JSON" << std::endl;

    return false;
  }

  const std::map<std::string, std::string> members = {
    {"adsShownHistory", "Array"},
    {"adUUID", "String"},
    {"adsUUIDSeen", "Object"},
    {"available", "Bool"},
    {"allowed", "Bool"},
    {"configured", "Bool"},
    {"currentSSID", "String"},
    {"expired", "String"},
    {"lastSearchTime", "Number"},
    {"lastShopTime", "Number"},
    {"lastUserActivity", "Number"},
    {"lastUserIdleStopTime", "Number"},
    {"locale", "String"},
    {"locales", "Array"},
    {"pageScoreHistory", "Array"},
    {"places", "Object"},
    {"score", "Number"},
    {"searchActivity", "Bool"},
    {"searchUrl", "String"},
    {"shopActivity", "Bool"},
    {"shopUrl", "String"},
    {"status", "String"}
  };

  // TODO(Terry Mancey): Refactor validateJson by moving to json_helper class
  for (auto& member : user_model.GetObject()) {
    std::string member_name = member.name.GetString();
    std::string member_type = _rapidjson_member_types[member.value.GetType()];

    if (members.find(member_name) == members.end()) {
      LOG(WARNING) "JSON " << member_name << " member not used" << std::endl;
      continue;
    }

    std::string type = members.at(member_name);
    if (type.compare(member_type) != 0) {
      LOG(WARNING) << "Invalid type for JSON member "
        << member_name << std::endl;
      return false;
    }
  }

  if (user_model.HasMember("adsShownHistory")) {
    for (auto& i : user_model["adsShownHistory"].GetArray()) {
      ads_shown_history.push_back(i.GetUint64());
    }
  }

  if (user_model.HasMember("adUUID")) {
    ad_uuid = user_model["adUUID"].GetString();
  }

  if (user_model.HasMember("adsUUIDSeen")) {
    for (auto& i : user_model["adsUUIDSeen"].GetObject()) {
      ads_uuid_seen.insert({i.name.GetString(), i.value.GetInt64()});
    }
  }

  if (user_model.HasMember("available")) {
    available = user_model["available"].GetBool();
  }

  if (user_model.HasMember("allowed")) {
    allowed = user_model["allowed"].GetBool();
  }

  if (user_model.HasMember("configured")) {
    configured = user_model["configured"].GetBool();
  }

  if (user_model.HasMember("currentSSID")) {
    current_ssid = user_model["currentSSID"].GetString();
  }

  if (user_model.HasMember("expired")) {
    expired = user_model["expired"].GetBool();
  }

  if (user_model.HasMember("lastSearchTime")) {
    last_search_time = user_model["lastSearchTime"].GetUint64();
  }

  if (user_model.HasMember("lastShopTime")) {
    last_shop_time = user_model["lastShopTime"].GetUint64();
  }

  if (user_model.HasMember("lastUserActivity")) {
    last_user_activity = user_model["lastUserActivity"].GetUint64();
  }

  if (user_model.HasMember("lastUserIdleStopTime")) {
    last_user_idle_stop_time = user_model["lastUserIdleStopTime"].GetUint64();
  }

  if (user_model.HasMember("locale")) {
    locale = user_model["locale"].GetString();
  }

  if (user_model.HasMember("locales")) {
    for (auto& i : user_model["locales"].GetArray()) {
      locales.push_back(i.GetString());
    }
  }

  if (user_model.HasMember("pageScoreHistory")) {
    for (auto& history : user_model["pageScoreHistory"].GetArray()) {
      std::vector<double> pageScores = {};

      for (auto& pageScore : history.GetArray()) {
        pageScores.push_back(pageScore.GetDouble());
      }

      page_score_history.push_back(pageScores);
    }
  }

  if (user_model.HasMember("places")) {
    for (auto& i : user_model["places"].GetObject()) {
      places.insert({i.name.GetString(), i.value.GetString()});
    }
  }

  if (user_model.HasMember("score")) {
    score = user_model["score"].GetDouble();
  }

  if (user_model.HasMember("searchActivity")) {
    search_activity = user_model["searchActivity"].GetBool();
  }

  if (user_model.HasMember("searchUrl")) {
    search_url = user_model["searchUrl"].GetString();
  }

  if (user_model.HasMember("shopActivity")) {
    shop_activity = user_model["shopActivity"].GetBool();
  }

  if (user_model.HasMember("shopUrl")) {
    shop_url = user_model["shopUrl"].GetString();
  }

  if (user_model.HasMember("status")) {
    status = user_model["status"].GetString();
  }

  return true;
}

void SaveToJson(JsonWriter& writer, const USER_MODEL_STATE& state) {
  writer.StartObject();

  writer.String("adsShownHistory");
  writer.StartArray();
  for (auto& i : state.ads_shown_history) {
    writer.Uint64(i);
  }
  writer.EndArray();

  writer.String("adUUID");
  writer.String(state.ad_uuid.c_str());

  writer.String("adsUUIDSeen");
  writer.StartObject();
  for (auto& i : state.ads_uuid_seen) {
    writer.String(i.first.c_str());
    writer.Int64(i.second);
  }
  writer.EndObject();

  writer.String("available");
  writer.Bool(state.available);

  writer.String("allowed");
  writer.Bool(state.allowed);

  writer.String("configured");
  writer.Bool(state.configured);

  writer.String("currentSSID");
  writer.String(state.current_ssid.c_str());

  writer.String("expired");
  writer.Bool(state.expired);

  writer.String("lastSearchTime");
  writer.Uint64(state.last_search_time);

  writer.String("lastShopTime");
  writer.Uint64(state.last_shop_time);

  writer.String("lastUserActivity");
  writer.Uint64(state.last_user_activity);

  writer.String("lastUserIdleStopTime");
  writer.Uint64(state.last_user_idle_stop_time);

  writer.String("locale");
  writer.String(state.locale.c_str());

  writer.String("locales");
  writer.StartArray();
  for (auto& i : state.locales) {
    writer.String(i.c_str());
  }
  writer.EndArray();

  writer.String("pageScoreHistory");
  writer.StartArray();
  for (auto& history : state.page_score_history) {
    writer.StartArray();
    for (auto& pageScore : history) {
      writer.Double(pageScore);
    }
    writer.EndArray();
  }
  writer.EndArray();

  writer.String("places");
  writer.StartObject();
  for (auto& i : state.places) {
    writer.String(i.first.c_str());
    writer.String(i.second.c_str());
  }
  writer.EndObject();

  writer.String("score");
  writer.Double(state.score);

  writer.String("searchActivity");
  writer.Bool(state.search_activity);

  writer.String("searchUrl");
  writer.String(state.search_url.c_str());

  writer.String("shopActivity");
  writer.Bool(state.shop_activity);

  writer.String("shopUrl");
  writer.String(state.shop_url.c_str());

  writer.String("status");
  writer.String(state.status.c_str());

  writer.EndObject();
}

}  // namespace state
