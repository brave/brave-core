/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/client_state.h"

#include "bat/ads/ad_history.h"
#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/time.h"

#include "base/strings/string_number_conversions.h"

namespace ads {

ClientState::ClientState() = default;

ClientState::ClientState(
    const ClientState& state) = default;

ClientState::~ClientState() = default;

const std::string ClientState::ToJson() {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

Result ClientState::FromJson(
    const std::string& json,
    std::string* error_description) {
  rapidjson::Document client;
  client.Parse(json.c_str());

  if (client.HasParseError()) {
    if (error_description) {
      *error_description = helper::JSON::GetLastError(&client);
    }

    return FAILED;
  }

  if (client.HasMember("adPreferences")) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    const auto& value = client["adPreferences"];
    if (!value.Accept(writer) ||
        ad_prefs.FromJson(buffer.GetString()) != SUCCESS) {
      return FAILED;
    }
  }

  if (client.HasMember("adsShownHistory")) {
    for (const auto& ad_shown : client["adsShownHistory"].GetArray()) {
      // adsShownHistory used to be an array of timestamps, so if
      // that's what we have here don't import them and we'll just
      // start fresh.
      if (ad_shown.IsUint64()) {
        continue;
      }
      AdHistory ad_history;
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      if (ad_shown.Accept(writer) &&
          ad_history.FromJson(buffer.GetString()) == SUCCESS) {
        ads_shown_history.push_back(ad_history);
      }
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

  if (client.HasMember("publisherAdsUUIDSeen")) {
    for (const auto& publisher_ad_uuid_seen :
        client["publisherAdsUUIDSeen"].GetObject()) {
      publisher_ads_uuid_seen.insert({publisher_ad_uuid_seen.name.GetString(),
          publisher_ad_uuid_seen.value.GetInt64()});
    }
  }

  if (client.HasMember("nextCheckServeAd")) {
    next_check_serve_ad_timestamp_in_seconds =
        client["nextCheckServeAd"].GetUint64();
  }

  if (client.HasMember("available")) {
    available = client["available"].GetBool();
  }

  if (client.HasMember("lastSearchTime")) {
    auto migrated_timestamp_in_seconds = Time::MigrateTimestampToDoubleT(
        client["lastSearchTime"].GetUint64());
    last_search_time = migrated_timestamp_in_seconds;
  }

  if (client.HasMember("lastShopTime")) {
    auto migrated_timestamp_in_seconds = Time::MigrateTimestampToDoubleT(
        client["lastShopTime"].GetUint64());
    last_shop_time = migrated_timestamp_in_seconds;
  }

  if (client.HasMember("lastUserActivity")) {
    auto migrated_timestamp_in_seconds = Time::MigrateTimestampToDoubleT(
        client["lastUserActivity"].GetUint64());
    last_user_activity = migrated_timestamp_in_seconds;
  }

  if (client.HasMember("lastUserIdleStopTime")) {
    auto migrated_timestamp_in_seconds = Time::MigrateTimestampToDoubleT(
        client["lastUserIdleStopTime"].GetUint64());
    last_user_idle_stop_time = migrated_timestamp_in_seconds;
  }

  if (client.HasMember("userModelLanguage")) {
    user_model_language = client["userModelLanguage"].GetString();
  }

  if (client.HasMember("userModelLanguages")) {
    for (const auto& language : client["userModelLanguages"].GetArray()) {
      user_model_languages.push_back(language.GetString());
    }
  }

  if (client.HasMember("lastPageClassification")) {
    last_page_classification = client["lastPageClassification"].GetString();
  }

  if (client.HasMember("pageScoreHistory")) {
    for (const auto& history : client["pageScoreHistory"].GetArray()) {
      std::vector<double> page_scores;

      for (const auto& page_score : history.GetArray()) {
        page_scores.push_back(page_score.GetDouble());
      }

      page_score_history.push_back(page_scores);
    }
  }

  if (client.HasMember("creativeSetHistory")) {
    for (const auto& creative_set : client["creativeSetHistory"].GetObject()) {
      std::deque<uint64_t> timestamps_in_seconds;

      for (const auto& timestamp_in_seconds : creative_set.value.GetArray()) {
        auto migrated_timestamp_in_seconds = Time::MigrateTimestampToDoubleT(
            timestamp_in_seconds.GetUint64());
        timestamps_in_seconds.push_back(migrated_timestamp_in_seconds);
      }

      std::string creative_set_id = creative_set.name.GetString();
      creative_set_history.insert({creative_set_id, timestamps_in_seconds});
    }
  }

  if (client.HasMember("adConversionHistory")) {
    for (const auto& conversion : client["adConversionHistory"].GetObject()) {
      std::deque<uint64_t> timestamps_in_seconds;

      for (const auto& timestamp_in_seconds : conversion.value.GetArray()) {
        timestamps_in_seconds.push_back(timestamp_in_seconds.GetUint64());
      }

      std::string creative_set_id = conversion.name.GetString();
      ad_conversion_history.insert({creative_set_id, timestamps_in_seconds});
    }
  }

  if (client.HasMember("campaignHistory")) {
    for (const auto& campaign : client["campaignHistory"].GetObject()) {
      std::deque<uint64_t> timestamps_in_seconds;

      for (const auto& timestamp_in_seconds : campaign.value.GetArray()) {
        auto migrated_timestamp_in_seconds = Time::MigrateTimestampToDoubleT(
            timestamp_in_seconds.GetUint64());
        timestamps_in_seconds.push_back(migrated_timestamp_in_seconds);
      }

      std::string campaign_id = campaign.name.GetString();
      campaign_history.insert({campaign_id, timestamps_in_seconds});
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

  if (client.HasMember("version_code")) {
    version_code = client["version_code"].GetString();
  }

  return SUCCESS;
}

void SaveToJson(JsonWriter* writer, const ClientState& state) {
  writer->StartObject();

  writer->String("adPreferences");
  SaveToJson(writer, state.ad_prefs);

  writer->String("adsShownHistory");
  writer->StartArray();
  for (const auto& ad_shown : state.ads_shown_history) {
    SaveToJson(writer, ad_shown);
  }
  writer->EndArray();

  writer->String("adUUID");
  writer->String(state.ad_uuid.c_str());

  writer->String("adsUUIDSeen");
  writer->StartObject();
  for (const auto& ad_uuid_seen : state.ads_uuid_seen) {
    writer->String(ad_uuid_seen.first.c_str());
    writer->Uint64(ad_uuid_seen.second);
  }
  writer->EndObject();

  writer->String("publisherAdsUUIDSeen");
  writer->StartObject();
  for (const auto& publisher_ad_uuid_seen : state.publisher_ads_uuid_seen) {
    writer->String(publisher_ad_uuid_seen.first.c_str());
    writer->Uint64(publisher_ad_uuid_seen.second);
  }
  writer->EndObject();

  writer->String("nextCheckServeAd");
  writer->Uint64(state.next_check_serve_ad_timestamp_in_seconds);

  writer->String("available");
  writer->Bool(state.available);

  writer->String("lastSearchTime");
  writer->Uint64(state.last_search_time);

  writer->String("lastShopTime");
  writer->Uint64(state.last_shop_time);

  writer->String("lastUserActivity");
  writer->Uint64(state.last_user_activity);

  writer->String("lastUserIdleStopTime");
  writer->Uint64(state.last_user_idle_stop_time);

  writer->String("userModelLanguage");
  writer->String(state.user_model_language.c_str());

  writer->String("userModelLanguages");
  writer->StartArray();
  for (const auto& language : state.user_model_languages) {
    writer->String(language.c_str());
  }
  writer->EndArray();

  writer->String("lastPageClassification");
  writer->String(state.last_page_classification.c_str());

  writer->String("pageScoreHistory");
  writer->StartArray();
  for (const auto& page_score : state.page_score_history) {
    writer->StartArray();
    for (const auto& score : page_score) {
      writer->Double(score);
    }
    writer->EndArray();
  }
  writer->EndArray();

  writer->String("creativeSetHistory");
  writer->StartObject();
  for (const auto& creative_set_id : state.creative_set_history) {
    writer->String(creative_set_id.first.c_str());
    writer->StartArray();
    for (const auto& timestamp_in_seconds : creative_set_id.second) {
      writer->Uint64(timestamp_in_seconds);
    }
    writer->EndArray();
  }
  writer->EndObject();

  writer->String("adConversionHistory");
  writer->StartObject();
  for (const auto& creative_set_id : state.ad_conversion_history) {
    writer->String(creative_set_id.first.c_str());
    writer->StartArray();
    for (const auto& timestamp_in_seconds : creative_set_id.second) {
      writer->Uint64(timestamp_in_seconds);
    }
    writer->EndArray();
  }
  writer->EndObject();

  writer->String("campaignHistory");
  writer->StartObject();
  for (const auto& campaign_id : state.campaign_history) {
    writer->String(campaign_id.first.c_str());
    writer->StartArray();
    for (const auto& timestamp_in_seconds : campaign_id.second) {
      writer->Uint64(timestamp_in_seconds);
    }
    writer->EndArray();
  }
  writer->EndObject();

  writer->String("score");
  writer->Double(state.score);

  writer->String("searchActivity");
  writer->Bool(state.search_activity);

  writer->String("searchUrl");
  writer->String(state.search_url.c_str());

  writer->String("shopActivity");
  writer->Bool(state.shop_activity);

  writer->String("shopUrl");
  writer->String(state.shop_url.c_str());

  writer->String("version_code");
  writer->String(state.version_code.c_str());

  writer->EndObject();
}

}  // namespace ads
