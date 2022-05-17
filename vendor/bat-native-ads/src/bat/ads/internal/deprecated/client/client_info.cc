/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/deprecated/client/client_info.h"

#include <vector>

#include "base/check.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/deprecated/json_helper.h"
#include "build/build_config.h"

namespace ads {

ClientInfo::ClientInfo() = default;

ClientInfo::ClientInfo(const ClientInfo& info) = default;

ClientInfo::~ClientInfo() = default;

std::string ClientInfo::ToJson() {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

bool ClientInfo::FromJson(const std::string& json) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  if (document.HasParseError()) {
    BLOG(1, helper::JSON::GetLastError(&document));
    return false;
  }

  if (document.HasMember("adPreferences")) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    const auto& value = document["adPreferences"];
    if (!value.Accept(writer) || !ad_preferences.FromJson(buffer.GetString())) {
      return false;
    }
  }

#if !BUILDFLAG(IS_IOS)
  if (document.HasMember("adsShownHistory")) {
    for (const auto& ad_shown : document["adsShownHistory"].GetArray()) {
      // adsShownHistory used to be an array of timestamps, so if that's what we
      // have here don't import them and we'll just start fresh.
      if (ad_shown.IsInt64()) {
        continue;
      }
      HistoryItemInfo history_item;
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      if (ad_shown.Accept(writer) &&
          history_item.FromJson(buffer.GetString())) {
        history.push_back(history_item);
      }
    }
  }
#endif

  if (document.HasMember("purchaseIntentSignalHistory")) {
    for (const auto& segment_history :
         document["purchaseIntentSignalHistory"].GetObject()) {
      std::string segment = segment_history.name.GetString();
      DCHECK(!segment.empty());

      std::vector<targeting::PurchaseIntentSignalHistoryInfo> histories;
      for (const auto& segment_history_item :
           segment_history.value.GetArray()) {
        targeting::PurchaseIntentSignalHistoryInfo history;
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        if (segment_history_item.Accept(writer) &&
            history.FromJson(buffer.GetString())) {
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
    const double timestamp = document["nextCheckServeAd"].GetDouble();
    serve_ad_at = base::Time::FromDoubleT(timestamp);
  }

  if (document.HasMember("textClassificationProbabilitiesHistory")) {
    for (const auto& probabilities :
         document["textClassificationProbabilitiesHistory"].GetArray()) {
      targeting::TextClassificationProbabilitiesMap new_probabilities;

      for (const auto& probability :
           probabilities["textClassificationProbabilities"].GetArray()) {
        const std::string segment = probability["segment"].GetString();
        DCHECK(!segment.empty());

        const double page_score = probability["pageScore"].GetDouble();

        new_probabilities.insert({segment, page_score});
      }

      text_classification_probabilities.push_back(new_probabilities);
    }
  }

  if (document.HasMember("version_code")) {
    version_code = document["version_code"].GetString();
  }

  return true;
}

void SaveToJson(JsonWriter* writer, const ClientInfo& info) {
  writer->StartObject();

  writer->String("adPreferences");
  SaveToJson(writer, info.ad_preferences);

  writer->String("adsShownHistory");
  writer->StartArray();
  for (const auto& item : info.history) {
    SaveToJson(writer, item);
  }
  writer->EndArray();

  writer->String("purchaseIntentSignalHistory");
  writer->StartObject();
  for (const auto& segment_history : info.purchase_intent_signal_history) {
    writer->String(segment_history.first.c_str());

    writer->StartArray();
    for (const auto& segment_history_item : segment_history.second) {
      writer->String(segment_history_item.ToJson().c_str());
    }
    writer->EndArray();
  }
  writer->EndObject();

  writer->String("seenAds");
  writer->StartObject();
  for (const auto& seen_ads : info.seen_ads) {
    const std::string& ad_type = seen_ads.first;
    writer->String(ad_type.c_str());
    writer->StartObject();

    for (const auto& seen_ad : seen_ads.second) {
      const std::string& creative_instance_id = seen_ad.first;
      writer->String(creative_instance_id.c_str());

      const bool was_seen = seen_ad.second;
      writer->Bool(was_seen);
    }

    writer->EndObject();
  }
  writer->EndObject();

  writer->String("seenAdvertisers");
  writer->StartObject();
  for (const auto& seen_advertisers : info.seen_advertisers) {
    const std::string& ad_type = seen_advertisers.first;
    writer->String(ad_type.c_str());
    writer->StartObject();

    for (const auto& seen_advertiser : seen_advertisers.second) {
      const std::string& advertiser_id = seen_advertiser.first;
      writer->String(advertiser_id.c_str());

      const bool was_seen = seen_advertiser.second;
      writer->Bool(was_seen);
    }

    writer->EndObject();
  }
  writer->EndObject();

  writer->String("nextCheckServeAd");
  writer->Double(info.serve_ad_at.ToDoubleT());

  writer->String("textClassificationProbabilitiesHistory");
  writer->StartArray();
  for (const auto& probabilities : info.text_classification_probabilities) {
    writer->StartObject();

    writer->String("textClassificationProbabilities");
    writer->StartArray();

    for (const auto& probability : probabilities) {
      writer->StartObject();

      writer->String("segment");
      const std::string segment = probability.first;
      DCHECK(!segment.empty());
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
  writer->String(info.version_code.c_str());

  writer->EndObject();
}

}  // namespace ads
