/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/client/client_info.h"

#include <utility>
#include <vector>

#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_signal_history_value_util.h"
#include "brave/components/brave_ads/core/public/history/history_item_value_util.h"
#include "build/build_config.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

ClientInfo::ClientInfo() = default;

ClientInfo::ClientInfo(const ClientInfo& other) = default;

ClientInfo& ClientInfo::operator=(const ClientInfo& other) = default;

ClientInfo::ClientInfo(ClientInfo&& other) noexcept = default;

ClientInfo& ClientInfo::operator=(ClientInfo&& other) noexcept = default;

ClientInfo::~ClientInfo() = default;

base::Value::Dict ClientInfo::ToValue() const {
  base::Value::Dict dict;

  dict.Set("adPreferences", ad_preferences.ToValue());

  dict.Set("adsShownHistory", HistoryItemsToValue(history_items));

  base::Value::Dict purchase_intent_signal_history_dict;
  for (const auto& [segment, history] : purchase_intent_signal_history) {
    base::Value::List list;
    for (const auto& item : history) {
      list.Append(PurchaseIntentSignalHistoryToValue(item));
    }

    purchase_intent_signal_history_dict.Set(segment, std::move(list));
  }
  dict.Set("purchaseIntentSignalHistory",
           std::move(purchase_intent_signal_history_dict));

  base::Value::Dict seen_ads_dict;
  for (const auto& [ad_type, ads] : seen_ads) {
    base::Value::Dict seen_ad_dict;
    for (const auto& [creative_instance_id, seen_ad] : ads) {
      seen_ad_dict.Set(creative_instance_id, seen_ad);
    }

    seen_ads_dict.Set(ad_type, std::move(seen_ad_dict));
  }
  dict.Set("seenAds", std::move(seen_ads_dict));

  base::Value::Dict seen_advertisers_dict;
  for (const auto& [ad_type, advertisers] : seen_advertisers) {
    base::Value::Dict seen_advertiser_dict;
    for (const auto& [advertiser_id, seen_advertiser] : advertisers) {
      seen_advertiser_dict.Set(advertiser_id, seen_advertiser);
    }

    seen_advertisers_dict.Set(ad_type, std::move(seen_advertiser_dict));
  }
  dict.Set("seenAdvertisers", std::move(seen_advertisers_dict));

  base::Value::List probabilities_history_list;
  for (const auto& item : text_classification_probabilities) {
    base::Value::List probabilities_list;

    for (const auto& [segmemt, page_score] : item) {
      CHECK(!segmemt.empty());

      probabilities_list.Append(
          base::Value::Dict()
              .Set("segment", segmemt)
              .Set("pageScore", base::NumberToString(page_score)));
    }

    probabilities_history_list.Append(base::Value::Dict().Set(
        "textClassificationProbabilities", std::move(probabilities_list)));
  }

  dict.Set("textClassificationProbabilitiesHistory",
           std::move(probabilities_history_list));

  return dict;
}

// TODO(https://github.com/brave/brave-browser/issues/26003): Reduce cognitive
// complexity.
bool ClientInfo::FromValue(const base::Value::Dict& dict) {
  if (const auto* const value = dict.FindDict("adPreferences")) {
    ad_preferences.FromValue(*value);
  }

#if !BUILDFLAG(IS_IOS)
  if (const auto* const value = dict.FindList("adsShownHistory")) {
    history_items = HistoryItemsFromValue(*value);
  }
#endif

  if (const auto* const value = dict.FindDict("purchaseIntentSignalHistory")) {
    for (const auto [segment, history] : *value) {
      const auto* items = history.GetIfList();
      if (!items) {
        continue;
      }

      std::vector<PurchaseIntentSignalHistoryInfo> histories;

      for (const auto& item : *items) {
        if (!item.is_dict()) {
          continue;
        }

        histories.push_back(
            PurchaseIntentSignalHistoryFromValue(item.GetDict()));
      }

      purchase_intent_signal_history.emplace(segment, histories);
    }
  }

  if (const auto* const value = dict.FindDict("seenAds")) {
    for (const auto [ad_type, ads] : *value) {
      if (!ads.is_dict()) {
        continue;
      }

      for (const auto [creative_instance_id, seen_ad] : ads.GetDict()) {
        CHECK(seen_ad.is_bool());
        seen_ads[ad_type][creative_instance_id] = seen_ad.GetBool();
      }
    }
  }

  if (const auto* const value = dict.FindDict("seenAdvertisers")) {
    for (const auto [ad_type, advertisers] : *value) {
      if (!advertisers.is_dict()) {
        continue;
      }

      for (const auto [advertiser_id, seen_advertiser] :
           advertisers.GetDict()) {
        CHECK(seen_advertiser.is_bool());
        seen_advertisers[ad_type][advertiser_id] = seen_advertiser.GetBool();
      }
    }
  }

  if (const auto* const value =
          dict.FindList("textClassificationProbabilitiesHistory")) {
    for (const auto& probability_history : *value) {
      if (!probability_history.is_dict()) {
        continue;
      }

      const auto* const probability_list =
          probability_history.GetDict().FindList(
              "textClassificationProbabilities");
      if (!probability_list) {
        continue;
      }

      TextClassificationProbabilityMap probabilities;

      for (const auto& item : *probability_list) {
        const auto* const item_dict = item.GetIfDict();
        if (!item_dict) {
          continue;
        }

        const std::string* const segment = item_dict->FindString("segment");
        if (!segment) {
          continue;
        }

        double page_score = 0.0;
        if (const auto page_score_value = dict.FindDouble("pageScore")) {
          page_score = *page_score_value;
        } else if (const auto* const legacy_page_score_value =
                       dict.FindString("pageScore")) {
          const bool success =
              base::StringToDouble(*legacy_page_score_value, &page_score);
          CHECK(success);
        }

        probabilities.insert({*segment, page_score});
      }

      text_classification_probabilities.push_back(probabilities);
    }
  }

  return true;
}

std::string ClientInfo::ToJson() const {
  std::string json;
  CHECK(base::JSONWriter::Write(ToValue(), &json));
  return json;
}

bool ClientInfo::FromJson(const std::string& json) {
  const absl::optional<base::Value> root =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!root || !root->is_dict()) {
    return false;
  }

  return FromValue(root->GetDict());
}

}  // namespace brave_ads
