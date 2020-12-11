/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/components/ftx/browser/ftx_json_parser.h"

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"

bool FTXJSONParser::GetFuturesDataFromJSON(
    const std::string& json,
    FTXFuturesData* data,
    const std::string& filter) {
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

  const base::Value* result = records_v->FindKey("result");
  if (!result || !result->is_list()) {
    return false;
  }

  for (const base::Value &asset : result->GetList()) {
    std::map<std::string, std::string> sub_data;
    const base::Value* group = asset.FindKey("group");

    if (!(group || group->is_string())) {
      continue;
    } else if (filter != group->GetString()) {
      continue;
    }

    const base::Value* bid = asset.FindKey("bid");
    const base::Value* name = asset.FindKey("name");
    const base::Value* day_change = asset.FindKey("change24h");
    const base::Value* day_volume = asset.FindKey("volumeUsd24h");

    if (!(bid && bid->is_double()) ||
        !(name && name->is_string()) ||
        !(day_change && day_change->is_double()) ||
        !(day_volume && day_volume->is_double())) {
      continue;
    }

    sub_data.insert({"price", std::to_string(bid->GetDouble())});
    sub_data.insert({"symbol", name->GetString()});
    sub_data.insert({"percentChange",
        std::to_string(day_change->GetDouble())});
    sub_data.insert({"dayVolume", std::to_string(day_volume->GetDouble())});
    data->push_back(sub_data);
  }

  return true;
}

bool FTXJSONParser::GetChartDataFromJSON(
    const std::string& json,
    FTXChartData* data) {
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

  const base::Value* result = records_v->FindKey("result");
  if (!result || !result->is_list()) {
    return false;
  }

  bool success = true;

  for (const base::Value &point : result->GetList()) {
    std::map<std::string, std::string> data_point;
    const base::Value* high = point.FindKey("high");
    const base::Value* low = point.FindKey("low");
    const base::Value* close = point.FindKey("close");

    if (!(high && high->is_double()) ||
        !(low && low->is_double()) ||
        !(close && close->is_double())) {
      success = false;
      break;
    }

    data_point.insert({"high", std::to_string(high->GetDouble())});
    data_point.insert({"low", std::to_string(low->GetDouble())});
    data_point.insert({"close", std::to_string(close->GetDouble())});

    data->push_back(data_point);
  }

  return success;
}
