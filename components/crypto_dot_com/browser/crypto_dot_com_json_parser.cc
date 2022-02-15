/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/components/crypto_dot_com/browser/crypto_dot_com_json_parser.h"

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"

void CryptoDotComJSONParser::CalculateAssetVolume(
    const double v,
    const double h,
    const double l,
    std::string* volume) {
  // Volume is v * ((h + l) / 2)
  double calc_volume = v * ((h + l) / 2.0);
  *volume = std::to_string(calc_volume);
}

bool CryptoDotComJSONParser::GetTickerInfoFromJSON(
    const std::string& json,
    CryptoDotComTickerInfo* info) {
  if (!info) {
    return false;
  }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;

  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::Value* response = records_v->FindKey("response");
  const base::Value* result = response->FindKey("result");
  const base::Value* data = result->FindKey("data");

  if (!response || !result || !data) {
    // Empty values on failed response
    info->insert({"volume", std::string()});
    info->insert({"price", std::string()});
    return false;
  }

  const base::Value* v = data->FindKey("v");
  const base::Value* h = data->FindKey("h");
  const base::Value* l = data->FindKey("l");
  const base::Value* price = data->FindKey("a");

  // Number could be double or int.
  if (!(v && (v->is_double() || v->is_int())) ||
      !(h && (h->is_double() || h->is_int())) ||
      !(l && (l->is_double() || l->is_int())) ||
      !(price && (price->is_double() || price->is_int()))) {
    info->insert({"volume", std::string()});
    info->insert({"price", std::string()});
    return false;
  }

  std::string volume;
  CalculateAssetVolume(
      v->GetDouble(), h->GetDouble(), l->GetDouble(), &volume);

  info->insert({"volume", volume});
  info->insert({"price", std::to_string(price->GetDouble())});

  return true;
}

bool CryptoDotComJSONParser::GetChartDataFromJSON(
    const std::string& json,
    CryptoDotComChartData* data) {
  if (!data) {
    return false;
  }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;

  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::Value* response = records_v->FindKey("response");
  if (!response) {
    return false;
  }

  const base::Value* result = response->FindKey("result");
  if (!result) {
    return false;
  }

  const base::Value* data_arr = result->FindKey("data");
  if (!data_arr || !data_arr->is_list()) {
    return false;
  }

  bool success = true;

  for (const base::Value& point : data_arr->GetListDeprecated()) {
    std::map<std::string, std::string> data_point;
    const base::Value* t = point.FindKey("t");
    const base::Value* o = point.FindKey("o");
    const base::Value* h = point.FindKey("h");
    const base::Value* l = point.FindKey("l");
    const base::Value* c = point.FindKey("c");
    const base::Value* v = point.FindKey("v");

    if (!(t && t->is_double()) ||
        !(o && o->is_double()) ||
        !(h && h->is_double()) ||
        !(l && l->is_double()) ||
        !(c && c->is_double()) ||
        !(v && v->is_double())) {
      success = false;
      break;
    }

    data_point.insert({"t", std::to_string(t->GetDouble())});
    data_point.insert({"o", std::to_string(o->GetDouble())});
    data_point.insert({"h", std::to_string(h->GetDouble())});
    data_point.insert({"l", std::to_string(l->GetDouble())});
    data_point.insert({"c", std::to_string(c->GetDouble())});
    data_point.insert({"v", std::to_string(v->GetDouble())});

    data->push_back(data_point);
  }

  return success;
}

bool CryptoDotComJSONParser::GetPairsFromJSON(
    const std::string& json,
    CryptoDotComSupportedPairs* pairs) {
  if (!pairs) {
    return false;
  }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;

  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::Value* response = records_v->FindKey("response");
  if (!response) {
    return false;
  }

  const base::Value* result = response->FindKey("result");
  if (!result) {
    return false;
  }

  const base::Value* instruments = result->FindKey("instruments");
  if (!instruments || !instruments->is_list()) {
    return false;
  }

  for (const base::Value& instrument : instruments->GetListDeprecated()) {
    std::map<std::string, std::string> instrument_data;
    const base::Value* pair = instrument.FindKey("instrument_name");
    const base::Value* quote = instrument.FindKey("quote_currency");
    const base::Value* base = instrument.FindKey("base_currency");

    if (!(pair && pair->is_string()) ||
        !(quote && quote->is_string()) ||
        !(base && base->is_string())) {
      continue;
    }

    instrument_data.insert({"pair", pair->GetString()});
    instrument_data.insert({"quote", quote->GetString()});
    instrument_data.insert({"base", base->GetString()});

    pairs->push_back(instrument_data);
  }

  return true;
}

bool CryptoDotComJSONParser::GetRankingsFromJSON(
    const std::string& json,
    CryptoDotComAssetRankings* rankings) {
  if (!rankings) {
    return false;
  }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;

  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::Value* response = records_v->FindKey("response");
  if (!response) {
    return false;
  }

  const base::Value* result = response->FindKey("result");
  if (!result) {
    return false;
  }

  std::vector<std::map<std::string, std::string>> gainers;
  std::vector<std::map<std::string, std::string>> losers;

  // Both gainers and losers are part of the "gainers" list
  const base::Value* rankings_list = result->FindKey("gainers");
  if (!rankings_list || !rankings_list->is_list()) {
    // Gainers and losers should return empty on a bad response
    rankings->insert({"gainers", gainers});
    rankings->insert({"losers", losers});
    return false;
  }

  for (const base::Value& ranking : rankings_list->GetListDeprecated()) {
    std::map<std::string, std::string> ranking_data;
    const base::Value* pair = ranking.FindKey("instrument_name");
    const base::Value* change = ranking.FindKey("percent_change");
    const base::Value* last = ranking.FindKey("last_price");

    if (!pair || !pair->is_string() ||
        !change || !change->is_string() ||
        !last || !last->is_string()) {
      continue;
    }

    double percent_double;
    const std::string pair_name = pair->GetString();
    const std::string percent_change = change->GetString();
    const std::string last_price = last->GetString();

    if (!base::StringToDouble(change->GetString(), &percent_double)) {
      continue;
    }

    ranking_data.insert({"pair", pair_name});
    ranking_data.insert({"percentChange", percent_change});
    ranking_data.insert({"lastPrice", last_price});

    if (percent_double < 0.0) {
      losers.push_back(ranking_data);
    } else {
      gainers.push_back(ranking_data);
    }
  }

  rankings->insert({"gainers", gainers});
  rankings->insert({"losers", losers});

  return true;
}
