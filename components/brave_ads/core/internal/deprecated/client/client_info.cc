/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/client/client_info.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/debug/dump_without_crashing.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_signal_history_value_util.h"
#include "brave/components/brave_ads/core/public/history/ad_history_value_util.h"
#include "build/build_config.h"  // IWYU pragma: keep

namespace brave_ads {

ClientInfo::ClientInfo() = default;

ClientInfo::ClientInfo(const ClientInfo& other) = default;

ClientInfo& ClientInfo::operator=(const ClientInfo& other) = default;

ClientInfo::ClientInfo(ClientInfo&& other) noexcept = default;

ClientInfo& ClientInfo::operator=(ClientInfo&& other) noexcept = default;

ClientInfo::~ClientInfo() = default;

base::Value::Dict ClientInfo::ToValue() const {
  base::Value::Dict dict =
      base::Value::Dict()
          .Set("adsShownHistory", AdHistoryToValue(ad_history));

  const base::TimeDelta time_window = kPurchaseIntentTimeWindow.Get();

  base::Value::Dict purchase_intent_signal_history_dict;
  for (const auto& [segment, history] : purchase_intent_signal_history) {
    base::Value::List list;
    for (const auto& item : history) {
      const base::Time decay_signal_at = item.at + time_window;
      if (base::Time::Now() < decay_signal_at) {
        list.Append(PurchaseIntentSignalHistoryToValue(item));
      }
    }

    purchase_intent_signal_history_dict.Set(segment, std::move(list));
  }
  dict.Set("purchaseIntentSignalHistory",
           std::move(purchase_intent_signal_history_dict));

  base::Value::List probabilities_history_list;
  for (const auto& item : text_classification_probabilities) {
    base::Value::List probabilities_list;

    for (const auto& [segment, page_score] : item) {
      CHECK(!segment.empty());

      probabilities_list.Append(
          base::Value::Dict()
              .Set("segment", segment)
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
#if !BUILDFLAG(IS_IOS)
  if (const auto* const value = dict.FindList("adsShownHistory")) {
    ad_history = AdHistoryFromValue(*value);
  }
#endif

  if (const auto* const value = dict.FindDict("purchaseIntentSignalHistory")) {
    for (const auto [segment, history] : *value) {
      const auto* const items = history.GetIfList();
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
          CHECK(base::StringToDouble(*legacy_page_score_value, &page_score));
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
  const std::optional<base::Value::Dict> dict = base::JSONReader::ReadDict(
      json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                base::JSONParserOptions::JSON_PARSE_RFC);
  if (!dict) {
    // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
    // potential defects using `DumpWithoutCrashing`.
    SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                              "Malformed client JSON state");
    base::debug::DumpWithoutCrashing();

    BLOG(0, "Malformed client JSON state");

    return false;
  }

  return FromValue(*dict);
}

}  // namespace brave_ads
