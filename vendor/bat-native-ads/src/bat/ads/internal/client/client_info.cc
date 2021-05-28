/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/client/client_info.h"

#include "base/time/time.h"
#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/logging.h"

namespace ads {

ClientInfo::ClientInfo() = default;

ClientInfo::ClientInfo(const ClientInfo& state) = default;

ClientInfo::~ClientInfo() = default;

std::string ClientInfo::ToJson() {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

Result ClientInfo::FromJson(const std::string& json) {
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

  if (document.HasMember("seenAds")) {
    for (const auto& seen_ad_list : document["seenAds"].GetObject()) {
      const std::string ad_type = seen_ad_list.name.GetString();

      for (const auto& seen_ad : seen_ad_list.value.GetObject()) {
        const std::string creative_instance_id = seen_ad.name.GetString();
        const bool has_seen = seen_ad.value.GetBool();

        seen_ads[ad_type][creative_instance_id] = has_seen;
      }
    }
  }

  if (document.HasMember("seenAdvertisers")) {
    for (const auto& seen_advertiser_list :
         document["seenAdvertisers"].GetObject()) {
      const std::string ad_type = seen_advertiser_list.name.GetString();

      for (const auto& seen_advertiser :
           seen_advertiser_list.value.GetObject()) {
        const std::string advertiser_id = seen_advertiser.name.GetString();
        const bool has_seen = seen_advertiser.value.GetBool();

        seen_advertisers[ad_type][advertiser_id] = has_seen;
      }
    }
  }

  if (document.HasMember("nextCheckServeAd")) {
    next_ad_serving_interval_timestamp =
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

  if (document.HasMember("version_code")) {
    version_code = document["version_code"].GetString();
  }

  return SUCCESS;
}

void SaveToJson(JsonWriter* writer, const ClientInfo& state) {
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

  writer->String("seenAds");
  writer->StartObject();
  for (const auto& seen_ads : state.seen_ads) {
    const std::string type = std::string(seen_ads.first);
    writer->String(type.c_str());
    writer->StartObject();

    for (const auto& seen_ad : seen_ads.second) {
      writer->String(seen_ad.first.c_str());
      writer->Bool(seen_ad.second);
    }

    writer->EndObject();
  }
  writer->EndObject();

  writer->String("seenAdvertisers");
  writer->StartObject();
  for (const auto& seen_advertisers : state.seen_advertisers) {
    const std::string type = std::string(seen_advertisers.first);
    writer->String(type.c_str());
    writer->StartObject();

    for (const auto& seen_advertiser : seen_advertisers.second) {
      writer->String(seen_advertiser.first.c_str());
      writer->Bool(seen_advertiser.second);
    }

    writer->EndObject();
  }
  writer->EndObject();

  writer->String("nextCheckServeAd");
  writer->Uint64(state.next_ad_serving_interval_timestamp);

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

  writer->String("version_code");
  writer->String(state.version_code.c_str());

  writer->EndObject();
}

}  // namespace ads
