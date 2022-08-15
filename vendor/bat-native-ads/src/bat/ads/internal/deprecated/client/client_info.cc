/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/deprecated/client/client_info.h"

#include <utility>
#include <vector>

#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "build/build_config.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

ClientInfo::ClientInfo() = default;

ClientInfo::ClientInfo(const ClientInfo& info) = default;

ClientInfo& ClientInfo::operator=(const ClientInfo& info) = default;

ClientInfo::~ClientInfo() = default;

base::Value::Dict ClientInfo::ToValue() const {
  base::Value::Dict dict;

  dict.Set("adPreferences", ad_preferences.ToValue());

  base::Value::List ads_shown_history;
  for (const auto& item : history) {
    ads_shown_history.Append(item.ToValue());
  }
  dict.Set("adsShownHistory", std::move(ads_shown_history));

  base::Value::Dict purchase_intent_dict;
  for (const auto& [key, value] : purchase_intent_signal_history) {
    base::Value::List history;
    for (const auto& segment_history_item : value) {
      history.Append(segment_history_item.ToValue());
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
    for (const auto& [key, value] : value) {
      advertiser.Set(key, value);
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
  dict.Set("version_code", version_code);
  return dict;
}

bool ClientInfo::FromValue(const base::Value::Dict& root) {
  if (const auto* value = root.FindDict("adPreferences")) {
    ad_preferences.FromValue(*value);
  }

#if !BUILDFLAG(IS_IOS)
  if (const auto* value = root.FindList("adsShownHistory")) {
    for (const auto& ad_shown : *value) {
      // adsShownHistory used to be an array of timestamps, so if that's what we
      // have here don't import them and we'll just start fresh.
      if (!ad_shown.is_dict()) {
        continue;
      }

      HistoryItemInfo history_item;
      history_item.FromValue(ad_shown.GetDict());
      history.push_back(history_item);
    }
  }
#endif

  if (const auto* value = root.FindDict("purchaseIntentSignalHistory")) {
    for (const auto [key, value] : *value) {
      std::vector<targeting::PurchaseIntentSignalHistoryInfo> histories;

      const auto* segment_history_items = value.GetIfList();
      if (!segment_history_items) {
        continue;
      }

      for (const auto& segment_history_item : *segment_history_items) {
        if (!segment_history_item.is_dict()) {
          continue;
        }

        targeting::PurchaseIntentSignalHistoryInfo history;
        history.FromValue(segment_history_item.GetDict());
        histories.push_back(history);
      }

      purchase_intent_signal_history.emplace(key, histories);
    }
  }

  if (const auto* value = root.FindDict("seenAds")) {
    for (const auto [list_key, list_value] : *value) {
      if (!list_value.is_dict()) {
        continue;
      }

      for (const auto [key, value] : list_value.GetDict()) {
        seen_ads[list_key][key] = value.GetBool();
      }
    }
  }

  if (const auto* value = root.FindDict("seenAdvertisers")) {
    for (const auto [list_key, list_value] : *value) {
      if (!list_value.is_dict()) {
        continue;
      }

      for (const auto [key, value] : list_value.GetDict()) {
        seen_advertisers[list_key][key] = value.GetBool();
      }
    }
  }

  if (const auto* value =
          root.FindList("textClassificationProbabilitiesHistory")) {
    for (const auto& probabilities : *value) {
      if (!probabilities.is_dict())
        continue;
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
        if (const auto value = root.FindDouble("pageScore")) {
          // Migrate legacy page score
          page_score = *value;
        } else if (const auto* value = root.FindString("pageScore")) {
          const bool success = base::StringToDouble(*value, &page_score);
          DCHECK(success);
        }

        new_probabilities.insert({*segment, page_score});
      }

      text_classification_probabilities.push_back(new_probabilities);
    }
  }

  if (const auto* value = root.FindString("version_code")) {
    version_code = *value;
  }

  return true;
}

std::string ClientInfo::ToJson() {
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
