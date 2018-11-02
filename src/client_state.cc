/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "client_state.h"
#include "json_helper.h"

namespace ads {

CLIENT_STATE::CLIENT_STATE() :
    ads_shown_history({}),
    ad_uuid(""),
    ads_uuid_seen({}),
    available(false),
    allowed(false),
    configured(false),
    current_ssid(""),
    expired(false),
    last_search_time(std::time(nullptr)),
    last_shop_time(std::time(nullptr)),
    last_user_activity(std::time(nullptr)),
    last_user_idle_stop_time(std::time(nullptr)),
    locale("en"),
    locales({}),
    page_score_history({}),
    places({}),
    score(0.0),
    search_activity(false),
    search_url(""),
    shop_activity(false),
    shop_url(""),
    status("") {}

CLIENT_STATE::CLIENT_STATE(const CLIENT_STATE& state) {
  ads_shown_history = state.ads_shown_history;
  ad_uuid = state.ad_uuid;
  ads_uuid_seen = state.ads_uuid_seen;
  available = state.available;
  allowed = state.allowed;
  configured = state.configured;
  current_ssid = state.current_ssid;
  expired = state.expired;
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

CLIENT_STATE::~CLIENT_STATE() = default;

bool CLIENT_STATE::LoadFromJson(const std::string& json) {
  rapidjson::Document client;
  client.Parse(json.c_str());

  if (client.HasParseError()) {
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
    {"expired", "Bool"},
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

  // TODO(Terry Mancey): Decouple validateJson into json_helper class
  for (const auto& member : client.GetObject()) {
    std::string member_name = member.name.GetString();
    auto member_type = _rapidjson_member_types[member.value.GetType()];

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

  if (client.HasMember("adsShownHistory")) {
    for (const auto& ad_shown : client["adsShownHistory"].GetArray()) {
      ads_shown_history.push_back(ad_shown.GetUint64());
    }
  }

  if (client.HasMember("adUUID")) {
    ad_uuid = client["adUUID"].GetString();
  }

  if (client.HasMember("adsUUIDSeen")) {
    for (const auto& ad_uuid_seen : client["adsUUIDSeen"].GetObject()) {
      ads_uuid_seen.insert({ad_uuid_seen.name.GetString(),
        ad_uuid_seen.value.GetInt64()});
    }
  }

  if (client.HasMember("available")) {
    available = client["available"].GetBool();
  }

  if (client.HasMember("allowed")) {
    allowed = client["allowed"].GetBool();
  }

  if (client.HasMember("configured")) {
    configured = client["configured"].GetBool();
  }

  if (client.HasMember("currentSSID")) {
    current_ssid = client["currentSSID"].GetString();
  }

  if (client.HasMember("expired")) {
    expired = client["expired"].GetBool();
  }

  if (client.HasMember("lastSearchTime")) {
    last_search_time = client["lastSearchTime"].GetUint64();
  }

  if (client.HasMember("lastShopTime")) {
    last_shop_time = client["lastShopTime"].GetUint64();
  }

  if (client.HasMember("lastUserActivity")) {
    last_user_activity = client["lastUserActivity"].GetUint64();
  }

  if (client.HasMember("lastUserIdleStopTime")) {
    last_user_idle_stop_time = client["lastUserIdleStopTime"].GetUint64();
  }

  if (client.HasMember("locale")) {
    locale = client["locale"].GetString();
  }

  if (client.HasMember("locales")) {
    for (const auto& locale : client["locales"].GetArray()) {
      locales.push_back(locale.GetString());
    }
  }

  if (client.HasMember("pageScoreHistory")) {
    for (const auto& history : client["pageScoreHistory"].GetArray()) {
      std::vector<double> pageScores = {};

      for (const auto& pageScore : history.GetArray()) {
        pageScores.push_back(pageScore.GetDouble());
      }

      page_score_history.push_back(pageScores);
    }
  }

  if (client.HasMember("places")) {
    for (const auto& place : client["places"].GetObject()) {
      places.insert({place.name.GetString(), place.value.GetString()});
    }
  }

  if (client.HasMember("score")) {
    score = client["score"].GetDouble();
  }

  if (client.HasMember("searchActivity")) {
    search_activity = client["searchActivity"].GetBool();
  }

  if (client.HasMember("searchUrl")) {
    search_url = client["searchUrl"].GetString();
  }

  if (client.HasMember("shopActivity")) {
    shop_activity = client["shopActivity"].GetBool();
  }

  if (client.HasMember("shopUrl")) {
    shop_url = client["shopUrl"].GetString();
  }

  if (client.HasMember("status")) {
    status = client["status"].GetString();
  }

  return true;
}

void SaveToJson(JsonWriter& writer, const CLIENT_STATE& state) {
  writer.StartObject();

  writer.String("adsShownHistory");
  writer.StartArray();
  for (const auto& ad_shown : state.ads_shown_history) {
    writer.Uint64(ad_shown);
  }
  writer.EndArray();

  writer.String("adUUID");
  writer.String(state.ad_uuid.c_str());

  writer.String("adsUUIDSeen");
  writer.StartObject();
  for (const auto& ad_uuid_seen : state.ads_uuid_seen) {
    writer.String(ad_uuid_seen.first.c_str());
    writer.Uint64(ad_uuid_seen.second);
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
  for (const auto& locale : state.locales) {
    writer.String(locale.c_str());
  }
  writer.EndArray();

  writer.String("pageScoreHistory");
  writer.StartArray();
  for (const auto& history : state.page_score_history) {
    writer.StartArray();
    for (const auto& pageScore : history) {
      writer.Double(pageScore);
    }
    writer.EndArray();
  }
  writer.EndArray();

  writer.String("places");
  writer.StartObject();
  for (const auto& place : state.places) {
    writer.String(place.first.c_str());
    writer.String(place.second.c_str());
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

}  // namespace ads
