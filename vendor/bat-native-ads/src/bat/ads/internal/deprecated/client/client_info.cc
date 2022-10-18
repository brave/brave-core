/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/deprecated/client/client_info.h"

#include <utility>
#include <vector>

#include "absl/types/optional.h"
#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "bat/ads/history_item_value_util.h"
#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_signal_history_value_util.h"
#include "build/build_config.h"  // IWYU pragma: keep

namespace ads {

ClientInfo::ClientInfo() = default;

ClientInfo::ClientInfo(const ClientInfo& other) = default;

ClientInfo& ClientInfo::operator=(const ClientInfo& other) = default;

ClientInfo::ClientInfo(ClientInfo&& other) noexcept = default;

ClientInfo& ClientInfo::operator=(ClientInfo&& other) noexcept = default;

ClientInfo::~ClientInfo() = default;

base::Value::Dict ClientInfo::ToValue() const {
  base::Value::Dict dict;

  dict.Set("adPreferences", ad_preferences.ToValue());

  base::Value::List ads_shown_history = HistoryItemsToValue(history_items);
  dict.Set("adsShownHistory", std::move(ads_shown_history));

  base::Value::Dict purchase_intent_dict;
  for (const auto& [key, value] : purchase_intent_signal_history) {
    base::Value::List history;
    for (const auto& segment_history_item : value) {
      history.Append(
          targeting::PurchaseIntentSignalHistoryToValue(segment_history_item));
    }
    purchase_intent_dict.Set(key, std::move(history));
  }
  dict.Set("purchaseIntentSignalHistory", std::move(purchase_intent_dict));

  base::Value::Dict seen_ads_dict;
  for (const auto& [key, value] : seen_ads) {
    base::Value::Dict ad;
    for (const auto& [ad_key, ad_value] : value) {
      ad.Set(ad_key, ad_value);
    }

    seen_ads_dict.Set(key, std::move(ad));
  }
  dict.Set("seenAds", std::move(seen_ads_dict));

  base::Value::Dict advertisers;
  for (const auto& [key, value] : seen_advertisers) {
    base::Value::Dict advertiser;
    for (const auto& [ad_key, ad_value] : value) {
      advertiser.Set(ad_key, ad_value);
    }
    advertisers.Set(key, std::move(advertiser));
  }
  dict.Set("seenAdvertisers", std::move(advertisers));

  base::Value::List probabilities_history;
  for (const auto& probabilities : text_classification_probabilities) {
    base::Value::Dict classification_probabilities;
    base::Value::List text_probabilities;
    for (const auto& [key, value] : probabilities) {
      base::Value::Dict prob;
      DCHECK(!key.empty());
      prob.Set("segment", key);
      prob.Set("pageScore", base::NumberToString(value));
      text_probabilities.Append(std::move(prob));
    }
    classification_probabilities.Set("textClassificationProbabilities",
                                     std::move(text_probabilities));
    probabilities_history.Append(std::move(classification_probabilities));
  }
  dict.Set("textClassificationProbabilitiesHistory",
           std::move(probabilities_history));
  return dict;
}

bool ClientInfo::FromValue(const base::Value::Dict& root) {
  if (const auto* value = root.FindDict("adPreferences")) {
    ad_preferences.FromValue(*value);
  }

#if !BUILDFLAG(IS_IOS)
  if (const auto* value = root.FindList("adsShownHistory")) {
    history_items = HistoryItemsFromValue(*value);
  }
#endif

  if (const auto* value = root.FindDict("purchaseIntentSignalHistory")) {
    for (const auto [history_key, history_value] : *value) {
      std::vector<targeting::PurchaseIntentSignalHistoryInfo> histories;

      const auto* segment_history_items = history_value.GetIfList();
      if (!segment_history_items) {
        continue;
      }

      for (const auto& segment_history_item : *segment_history_items) {
        if (!segment_history_item.is_dict()) {
          continue;
        }

        const targeting::PurchaseIntentSignalHistoryInfo history =
            targeting::PurchaseIntentSignalHistoryFromValue(
                segment_history_item.GetDict());
        histories.push_back(history);
      }

      purchase_intent_signal_history.emplace(history_key, histories);
    }
  }

  if (const auto* value = root.FindDict("seenAds")) {
    for (const auto [list_key, list_value] : *value) {
      if (!list_value.is_dict()) {
        continue;
      }

      for (const auto [key_seen_ads, value_seen_ads] : list_value.GetDict()) {
        seen_ads[list_key][key_seen_ads] = value_seen_ads.GetBool();
      }
    }
  }

  if (const auto* value = root.FindDict("seenAdvertisers")) {
    for (const auto [list_key, list_value] : *value) {
      if (!list_value.is_dict()) {
        continue;
      }

      for (const auto [key_seen_advertisers, value_seen_advertisers] :
           list_value.GetDict()) {
        seen_advertisers[list_key][key_seen_advertisers] =
            value_seen_advertisers.GetBool();
      }
    }
  }

  if (const auto* value =
          root.FindList("textClassificationProbabilitiesHistory")) {
    for (const auto& probabilities : *value) {
      if (!probabilities.is_dict()) {
        continue;
      }
      const auto* probability_list =
          probabilities.GetDict().FindList("textClassificationProbabilities");
      if (!probability_list) {
        continue;
      }

      targeting::TextClassificationProbabilityMap new_probabilities;

      for (const auto& probability : *probability_list) {
        const base::Value::Dict* dict = probability.GetIfDict();
        if (!dict) {
          continue;
        }

        const std::string* segment = dict->FindString("segment");
        if (!segment) {
          continue;
        }

        double page_score = 0.0;
        if (const auto page_score_value_double = root.FindDouble("pageScore")) {
          // Migrate legacy page score
          page_score = *page_score_value_double;
        } else if (const auto* page_score_value_string =
                       root.FindString("pageScore")) {
          const bool success =
              base::StringToDouble(*page_score_value_string, &page_score);
          DCHECK(success);
        }

        new_probabilities.insert({*segment, page_score});
      }

      text_classification_probabilities.push_back(new_probabilities);
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

}  // namespace ads
