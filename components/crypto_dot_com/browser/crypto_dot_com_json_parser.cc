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

namespace {

absl::optional<double> ConvertValueToDouble(const base::Value* value) {
  if (!value) {
    return absl::nullopt;
  }

  if (value->is_double() || value->is_int()) {
    return value->GetDouble();
  }

  if (value->is_string()) {
    double result;
    if (base::StringToDouble(value->GetString(), &result)) {
      return result;
    }
  }

  return absl::nullopt;
}

}  // namespace

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

  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v || !records_v->is_dict()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::Value::Dict* response = records_v->GetDict().FindDict("response");
  const base::Value::Dict* result =
      response ? response->FindDict("result") : nullptr;
  const base::Value::Dict* data = result ? result->FindDict("data") : nullptr;
  if (!data && result) {
    // Response can contain "data" as array.
    const base::Value::List* data_list = result->FindList("data");
    if (data_list && data_list->size() > 0) {
      data = data_list->front().GetIfDict();
    }
  }

  if (!response || !result || !data) {
    // Empty values on failed response
    info->insert({"volume", std::string()});
    info->insert({"price", std::string()});
    return false;
  }

  const absl::optional<double> v = ConvertValueToDouble(data->Find("v"));
  const absl::optional<double> h = ConvertValueToDouble(data->Find("h"));
  const absl::optional<double> l = ConvertValueToDouble(data->Find("l"));
  const absl::optional<double> price = ConvertValueToDouble(data->Find("a"));

  if (!v || !h || !l || !price) {
    info->insert({"volume", std::string()});
    info->insert({"price", std::string()});
    return false;
  }

  std::string volume;
  CalculateAssetVolume(*v, *h, *l, &volume);

  info->insert({"volume", volume});
  info->insert({"price", std::to_string(*price)});

  return true;
}

bool CryptoDotComJSONParser::GetChartDataFromJSON(
    const std::string& json,
    CryptoDotComChartData* data) {
  if (!data) {
    return false;
  }

  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v || !records_v->is_dict()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::Value::Dict* response = records_v->GetDict().FindDict("response");
  if (!response) {
    return false;
  }

  const base::Value::Dict* result = response->FindDict("result");
  if (!result) {
    return false;
  }

  const base::Value::List* data_arr = result->FindList("data");
  if (!data_arr) {
    return false;
  }

  bool success = true;

  for (const base::Value& point : *data_arr) {
    std::map<std::string, std::string> data_point;
    const absl::optional<double> t = ConvertValueToDouble(point.FindKey("t"));
    const absl::optional<double> o = ConvertValueToDouble(point.FindKey("o"));
    const absl::optional<double> h = ConvertValueToDouble(point.FindKey("h"));
    const absl::optional<double> l = ConvertValueToDouble(point.FindKey("l"));
    const absl::optional<double> c = ConvertValueToDouble(point.FindKey("c"));
    const absl::optional<double> v = ConvertValueToDouble(point.FindKey("v"));

    if (!t || !o || !h || !l || !c || !v) {
      success = false;
      break;
    }

    data_point.insert({"t", std::to_string(*t)});
    data_point.insert({"o", std::to_string(*o)});
    data_point.insert({"h", std::to_string(*h)});
    data_point.insert({"l", std::to_string(*l)});
    data_point.insert({"c", std::to_string(*c)});
    data_point.insert({"v", std::to_string(*v)});

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

  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v || !records_v->is_dict()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::Value::Dict* response = records_v->GetDict().FindDict("response");
  if (!response) {
    return false;
  }

  const base::Value::Dict* result = response->FindDict("result");
  if (!result) {
    return false;
  }

  const base::Value::List* instruments = result->FindList("instruments");
  if (!instruments) {
    return false;
  }

  for (const base::Value& instrument : *instruments) {
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

  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v || !records_v->is_dict()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::Value::Dict* response = records_v->GetDict().FindDict("response");
  const base::Value::Dict* result =
      response ? response->FindDict("result") : nullptr;

  std::vector<std::map<std::string, std::string>> gainers;
  std::vector<std::map<std::string, std::string>> losers;

  // Both gainers and losers are part of the "gainers" list
  const base::Value::List* rankings_list =
      result ? result->FindList("gainers") : nullptr;
  if (!rankings_list) {
    // Gainers and losers should return empty on a bad response
    rankings->insert({"gainers", gainers});
    rankings->insert({"losers", losers});
    return false;
  }

  for (const base::Value& ranking : *rankings_list) {
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
