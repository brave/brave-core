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
    std::map<std::string, std::string>* info) {
  if (!info) {
    return false;
  }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  base::Optional<base::Value>& records_v = value_with_error.value;

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

  const base::Value* data = result->FindKey("data");
  if (!data) {
    return false;
  }

  const base::Value* v = data->FindKey("v");
  const base::Value* h = data->FindKey("h");
  const base::Value* l = data->FindKey("l");
  const base::Value* price = data->FindKey("a");

  if (!(v && v->is_double()) ||
      !(h && h->is_double()) ||
      !(l && l->is_double()) ||
      !(price && price->is_double())) {
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
    std::vector<std::map<std::string, std::string>>* data) {
  if (!data) {
    return false;
  }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  base::Optional<base::Value>& records_v = value_with_error.value;

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

  for (const base::Value &point : data_arr->GetList()) {
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
    std::vector<std::map<std::string, std::string>>* pairs) {
  if (!pairs) {
    return false;
  }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  base::Optional<base::Value>& records_v = value_with_error.value;

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

  bool success = true;

  for (const base::Value &instrument : instruments->GetList()) {
    std::map<std::string, std::string> instrument_data;
    const base::Value* pair = instrument.FindKey("instrument_name");
    const base::Value* quote = instrument.FindKey("quote_currency");
    const base::Value* base = instrument.FindKey("base_currency");

    if (!(pair && pair->is_string()) ||
        !(quote && quote->is_string()) ||
        !(base && base->is_string())) {
      success = false;
      break;
    }

    instrument_data.insert({"pair", pair->GetString()});
    instrument_data.insert({"quote", quote->GetString()});
    instrument_data.insert({"base", base->GetString()});

    pairs->push_back(instrument_data);
  }

  return success;
}
