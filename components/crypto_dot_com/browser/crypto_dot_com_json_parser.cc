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