/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/client_state.h"

#include "bat/ads/ad_history.h"
#include "bat/ads/purchase_intent_signal_history.h"
#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/time_util.h"

#include "base/strings/string_number_conversions.h"

namespace ads {

ClientState::ClientState() = default;

ClientState::ClientState(
    const ClientState& state) = default;

ClientState::~ClientState() = default;

std::string ClientState::ToJson() {
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

  if (client.HasMember("purchaseIntentSignalHistory")) {
    for (const auto& segment_history :
        client["purchaseIntentSignalHistory"].GetObject()) {
      std::string segment = segment_history.name.GetString();
      std::deque<PurchaseIntentSignalHistory> histories;
      for (const auto& segment_history_item :
          segment_history.value.GetArray()) {
        PurchaseIntentSignalHistory history;
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        if (segment_history_item.Accept(writer) &&
            history.FromJson(buffer.GetString()) == SUCCESS) {
          histories.push_back(history);
        }
      }
      purchase_intent_signal_history.insert({segment, histories});
    }
  }


  if (client.HasMember("adUUID")) {
    ad_uuid = client["adUUID"].GetString();
  }

  if (client.HasMember("adsUUIDSeen")) {
    for (const auto& seen_ad_notification : client["adsUUIDSeen"].GetObject()) {
      seen_ad_notifications.insert({seen_ad_notification.name.GetString(),
          seen_ad_notification.value.GetInt64()});
    }
  }

  if (client.HasMember("advertisersUUIDSeen")) {
    for (const auto& seen_advertiser :
        client["advertisersUUIDSeen"].GetObject()) {
      seen_advertisers.insert({seen_advertiser.name.GetString(),
          seen_advertiser.value.GetInt64()});
    }
  }

  if (client.HasMember("nextCheckServeAd")) {
    next_check_serve_ad_timestamp_in_seconds =
        client["nextCheckServeAd"].GetUint64();
  }

  if (client.HasMember("available")) {
    available = client["available"].GetBool();
  }

  if (client.HasMember("userModelLanguage")) {
    user_model_language = client["userModelLanguage"].GetString();
  }

  if (client.HasMember("userModelLanguages")) {
    for (const auto& language : client["userModelLanguages"].GetArray()) {
      user_model_languages.push_back(language.GetString());
    }
  }

  if (client.HasMember("pageProbabilitiesHistory")) {
    for (const auto& page_probabilities :
        client["pageProbabilitiesHistory"].GetArray()) {
      classification::PageProbabilitiesMap new_page_probabilities;

      for (const auto& page_probability :
          page_probabilities["pageProbabilities"].GetArray()) {
        const std::string category = page_probability["category"].GetString();
        const double page_score = page_probability["pageScore"].GetDouble();

        new_page_probabilities.insert({category, page_score});
      }

      page_probabilities_history.push_back(new_page_probabilities);
    }
  }

  if (client.HasMember("creativeSetHistory")) {
    for (const auto& creative_set : client["creativeSetHistory"].GetObject()) {
      std::deque<uint64_t> timestamps_in_seconds;

      for (const auto& timestamp_in_seconds : creative_set.value.GetArray()) {
        auto migrated_timestamp_in_seconds =
            MigrateTimestampToDoubleT(timestamp_in_seconds.GetUint64());
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
        auto migrated_timestamp_in_seconds =
            MigrateTimestampToDoubleT(timestamp_in_seconds.GetUint64());
        timestamps_in_seconds.push_back(migrated_timestamp_in_seconds);
      }

      std::string campaign_id = campaign.name.GetString();
      campaign_history.insert({campaign_id, timestamps_in_seconds});
    }
  }

  if (client.HasMember("score")) {
    score = client["score"].GetDouble();
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

  writer->String("purchaseIntentSignalHistory");
  writer->StartObject();
  for (const auto& segment_history : state.purchase_intent_signal_history) {
    writer->String(segment_history.first.c_str());

    writer->StartArray();
    for (const auto& segment_history_item : segment_history.second) {
      SaveToJson(writer, segment_history_item);
    }
    writer->EndArray();
  }
  writer->EndObject();

  writer->String("adUUID");
  writer->String(state.ad_uuid.c_str());

  writer->String("adsUUIDSeen");
  writer->StartObject();
  for (const auto& seen_ad_notification : state.seen_ad_notifications) {
    writer->String(seen_ad_notification.first.c_str());
    writer->Uint64(seen_ad_notification.second);
  }
  writer->EndObject();

  writer->String("advertisersUUIDSeen");
  writer->StartObject();
  for (const auto& seen_advertiser : state.seen_advertisers) {
    writer->String(seen_advertiser.first.c_str());
    writer->Uint64(seen_advertiser.second);
  }
  writer->EndObject();

  writer->String("nextCheckServeAd");
  writer->Uint64(state.next_check_serve_ad_timestamp_in_seconds);

  writer->String("available");
  writer->Bool(state.available);

  writer->String("userModelLanguage");
  writer->String(state.user_model_language.c_str());

  writer->String("userModelLanguages");
  writer->StartArray();
  for (const auto& language : state.user_model_languages) {
    writer->String(language.c_str());
  }
  writer->EndArray();

  writer->String("pageProbabilitiesHistory");
  writer->StartArray();
  for (const auto& page_probabilities : state.page_probabilities_history) {
    writer->StartObject();

    writer->String("pageProbabilities");
    writer->StartArray();

    for (const auto& page_probability : page_probabilities) {
      writer->StartObject();

      writer->String("category");
      const std::string category = page_probability.first;
      writer->String(category.c_str());

      writer->String("pageScore");
      const double page_score = page_probability.second;
      writer->Double(page_score);

      writer->EndObject();
    }

    writer->EndArray();

    writer->EndObject();
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

  writer->String("version_code");
  writer->String(state.version_code.c_str());

  writer->EndObject();
}

}  // namespace ads
