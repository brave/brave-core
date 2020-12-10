/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/client/client_info.h"

#include "base/time/time.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/json_helper.h"

namespace ads {

ClientInfo::ClientInfo() = default;

ClientInfo::ClientInfo(
    const ClientInfo& state) = default;

ClientInfo::~ClientInfo() = default;

std::string ClientInfo::ToJson() {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

Result ClientInfo::FromJson(
    const std::string& json) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  if (document.HasParseError()) {
    BLOG(1, helper::JSON::GetLastError(&document));
    return FAILED;
  }

  if (document.HasMember("adPreferences")) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    const auto& value = document["adPreferences"];
    if (!value.Accept(writer) ||
        ad_preferences.FromJson(buffer.GetString()) != SUCCESS) {
      return FAILED;
    }
  }

  if (document.HasMember("adsShownHistory")) {
    for (const auto& ad_shown : document["adsShownHistory"].GetArray()) {
      // adsShownHistory used to be an array of timestamps, so if
      // that's what we have here don't import them and we'll just
      // start fresh.
      if (ad_shown.IsUint64()) {
        continue;
      }
      AdHistoryInfo ad_history;
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      if (ad_shown.Accept(writer) &&
          ad_history.FromJson(buffer.GetString()) == SUCCESS) {
        ads_shown_history.push_back(ad_history);
      }
    }
  }

  if (document.HasMember("purchaseIntentSignalHistory")) {
    for (const auto& segment_history :
        document["purchaseIntentSignalHistory"].GetObject()) {
      std::string segment = segment_history.name.GetString();
      std::deque<PurchaseIntentSignalHistoryInfo> histories;
      for (const auto& segment_history_item :
          segment_history.value.GetArray()) {
        PurchaseIntentSignalHistoryInfo history;
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

  if (document.HasMember("adsUUIDSeen")) {
    for (const auto& seen_ad_notification :
        document["adsUUIDSeen"].GetObject()) {
      seen_ad_notifications.insert({seen_ad_notification.name.GetString(),
          seen_ad_notification.value.GetInt64()});
    }
  }

  if (document.HasMember("advertisersUUIDSeen")) {
    for (const auto& seen_advertiser :
        document["advertisersUUIDSeen"].GetObject()) {
      seen_advertisers.insert({seen_advertiser.name.GetString(),
          seen_advertiser.value.GetInt64()});
    }
  }

  if (document.HasMember("nextCheckServeAd")) {
    next_ad_serving_interval_timestamp_ =
        document["nextCheckServeAd"].GetUint64();
  }

  if (document.HasMember("textClassificationProbabilitiesHistory")) {
    for (const auto& probabilities :
        document["textClassificationProbabilitiesHistory"].GetArray()) {
      TextClassificationProbabilitiesMap new_probabilities;

      for (const auto& probability :
          probabilities["textClassificationProbabilities"].GetArray()) {
        const std::string segment = probability["segment"].GetString();
        const double page_score = probability["pageScore"].GetDouble();

        new_probabilities.insert({segment, page_score});
      }

      text_classification_probabilities.push_back(new_probabilities);
    }
  }

  return SUCCESS;
}

void SaveToJson(
    JsonWriter* writer,
    const ClientInfo& state) {
  writer->StartObject();

  writer->String("adPreferences");
  SaveToJson(writer, state.ad_preferences);

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
  writer->Uint64(state.next_ad_serving_interval_timestamp_);

  writer->String("textClassificationProbabilitiesHistory");
  writer->StartArray();
  for (const auto& probabilities : state.text_classification_probabilities) {
    writer->StartObject();

    writer->String("textClassificationProbabilities");
    writer->StartArray();

    for (const auto& probability : probabilities) {
      writer->StartObject();

      writer->String("segment");
      const std::string segment = probability.first;
      writer->String(segment.c_str());

      writer->String("pageScore");
      const double page_score = probability.second;
      writer->Double(page_score);

      writer->EndObject();
    }

    writer->EndArray();

    writer->EndObject();
  }
  writer->EndArray();

  writer->EndObject();
}

}  // namespace ads
