/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../include/state_user_model.h"

namespace ads_bat_client {

USER_MODEL_STATE_ST::USER_MODEL_STATE_ST() :
    adsShownHistory({}),
    adUUID(""),
    adsUUIDSeen({}),
    available(false),
    allowed(false),
    // catalog({}),
    configured(false),
    currentSSID(""),
    expired(false),
    finalContactTimestamp(std::time(nullptr)),
    firstContactTimestamp(std::time(nullptr)),
    lastSearchTime(std::time(nullptr)),
    lastShopTime(std::time(nullptr)),
    lastUserActivity(std::time(nullptr)),
    lastUserIdleStopTime(std::time(nullptr)),
    locale(""),
    locales({}),
    pageScoreHistory({}),
    places({}),
    score(0.0),
    searchActivity(false),
    searchUrl(""),
    shopActivity(false),
    shopUrl(""),
    status("") {}

USER_MODEL_STATE_ST::USER_MODEL_STATE_ST(const USER_MODEL_STATE_ST& state) {
  adsShownHistory = state.adsShownHistory;
  adUUID = state.adUUID;
  adsUUIDSeen = state.adsUUIDSeen;
  available = state.available;
  allowed = state.allowed;
  // catalog = state.catalog;
  configured = state.configured;
  currentSSID = state.currentSSID;
  expired = state.expired;
  finalContactTimestamp = state.finalContactTimestamp;
  firstContactTimestamp = state.firstContactTimestamp;
  lastSearchTime = state.lastSearchTime;
  lastShopTime = state.lastShopTime;
  lastUserActivity = state.lastUserActivity;
  lastUserIdleStopTime = state.lastUserIdleStopTime;
  locale = state.locale;
  locales = state.locales;
  pageScoreHistory = state.pageScoreHistory;
  places = state.places;
  score = state.score;
  searchActivity = state.searchActivity;
  searchUrl = state.searchUrl;
  shopActivity = state.shopActivity;
  shopUrl = state.shopUrl;
  status = state.status;
}

USER_MODEL_STATE_ST::~USER_MODEL_STATE_ST() = default;

bool USER_MODEL_STATE_ST::LoadFromJson(const std::string& json) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  if (document.HasParseError()) {
    return false;
  }

  bool success =
    document.HasMember("adsShownHistory") &&
      document["adsShownHistory"].IsArray() &&
    document.HasMember("adUUID") &&
      document["adUUID"].IsString() &&
    document.HasMember("adsUUIDSeen") &&
      document["adsUUIDSeen"].IsObject() &&
    document.HasMember("available") &&
      document["available"].IsBool() &&
    document.HasMember("allowed") &&
      document["allowed"].IsBool() &&
    // document.HasMember("catalog") &&
    //   document["catalog"].IsObject() &&
    document.HasMember("configured") &&
      document["configured"].IsBool() &&
    document.HasMember("currentSSID") &&
      document["currentSSID"].IsString() &&
    document.HasMember("expired") &&
      document["expired"].IsBool() &&
    document.HasMember("finalContactTimestamp") &&
      document["finalContactTimestamp"].IsUint64() &&
    document.HasMember("firstContactTimestamp") &&
      document["firstContactTimestamp"].IsUint64() &&
    document.HasMember("lastSearchTime") &&
      document["lastSearchTime"].IsUint64() &&
    document.HasMember("lastShopTime") &&
      document["lastShopTime"].IsUint64() &&
    document.HasMember("lastUserActivity") &&
      document["lastUserActivity"].IsUint64() &&
    document.HasMember("lastUserIdleStopTime") &&
      document["lastUserIdleStopTime"].IsUint64() &&
    document.HasMember("locale") &&
      document["locale"].IsString() &&
    document.HasMember("locales") &&
      document["locales"].IsArray() &&
    document.HasMember("pageScoreHistory") &&
      document["pageScoreHistory"].IsArray() &&
    document.HasMember("places") &&
      document["places"].IsObject() &&
    document.HasMember("score") &&
      document["score"].IsDouble() &&
    document.HasMember("searchActivity") &&
      document["searchActivity"].IsBool() &&
    document.HasMember("searchUrl") &&
      document["searchUrl"].IsString() &&
    document.HasMember("shopActivity") &&
      document["shopActivity"].IsBool() &&
    document.HasMember("shopUrl") &&
      document["shopUrl"].IsString() &&
    document.HasMember("status") &&
      document["status"].IsString();

  if (success) {
    for (auto& i : document["adsShownHistory"].GetArray()) {
      adsShownHistory.push_back(i.GetUint64());
    }

    adUUID = document["adUUID"].GetString();

    for (auto& i : document["adsUUIDSeen"].GetObject()) {
      adsUUIDSeen.insert(std::make_pair(i.name.GetString(), i.value.GetBool()));
    }

    available = document["available"].GetBool();
    allowed = document["allowed"].GetBool();

    // TODO(Terry Mancey): catalog
    // catalog = document["catalog"].GetObject();

    configured = document["configured"].GetBool();
    currentSSID = document["currentSSID"].GetString();
    expired = document["expired"].GetBool();
    finalContactTimestamp = document["finalContactTimestamp"].GetUint64();
    firstContactTimestamp = document["firstContactTimestamp"].GetUint64();
    lastSearchTime = document["lastSearchTime"].GetUint64();
    lastShopTime = document["lastShopTime"].GetUint64();
    lastUserActivity = document["lastUserActivity"].GetUint64();
    lastUserIdleStopTime = document["lastUserIdleStopTime"].GetUint64();
    locale = document["locale"].GetString();

    for (auto& i : document["locales"].GetArray()) {
      locales.push_back(i.GetString());
    }

    for (auto& i : document["pageScoreHistory"].GetArray()) {
      pageScoreHistory.push_back(i.GetDouble());
    }

    for (auto& i : document["places"].GetObject()) {
      places.insert(std::make_pair(i.name.GetString(), i.value.GetString()));
    }

    score = document["score"].GetDouble();
    searchActivity = document["searchActivity"].GetBool();
    searchUrl = document["searchUrl"].GetString();

    shopActivity = document["shopActivity"].GetBool();
    shopUrl = document["shopUrl"].GetString();
    status = document["status"].GetString();
  }

  return success;
}

void SaveToJson(JsonWriter& writer, const USER_MODEL_STATE_ST& state) {
  writer.StartObject();

  writer.String("adsShownHistory");
  writer.StartArray();
  for (auto& i : state.adsShownHistory) {
    writer.Uint64(i);
  }
  writer.EndArray();

  writer.String("adUUID");
  writer.String(state.adUUID.c_str());

  writer.String("adsUUIDSeen");
  writer.StartObject();
  for (auto& i : state.adsUUIDSeen) {
    writer.String(i.first.c_str());
    writer.Bool(i.second);
  }
  writer.EndObject();

  writer.String("available");
  writer.Bool(state.available);

  writer.String("allowed");
  writer.Bool(state.allowed);

  // TODO(Terry Mancey): std::map<std::string, std::any> catalog;

  writer.String("configured");
  writer.Bool(state.configured);

  writer.String("currentSSID");
  writer.String(state.currentSSID.c_str());

  writer.String("expired");
  writer.Bool(state.expired);

  writer.String("finalContactTimestamp");
  writer.Uint64(state.finalContactTimestamp);

  writer.String("firstContactTimestamp");
  writer.Uint64(state.firstContactTimestamp);

  writer.String("lastSearchTime");
  writer.Uint64(state.lastSearchTime);

  writer.String("lastShopTime");
  writer.Uint64(state.lastShopTime);

  writer.String("lastUserActivity");
  writer.Uint64(state.lastUserActivity);

  writer.String("lastUserIdleStopTime");
  writer.Uint64(state.lastUserIdleStopTime);

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
  for (auto& i : state.pageScoreHistory) {
    writer.Double(i);
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
  writer.Bool(state.searchActivity);

  writer.String("searchUrl");
  writer.String(state.searchUrl.c_str());

  writer.String("shopActivity");
  writer.Bool(state.shopActivity);

  writer.String("shopUrl");
  writer.String(state.shopUrl.c_str());

  writer.String("status");
  writer.String(state.status.c_str());

  writer.EndObject();
}

}  // namespace ads_bat_client
