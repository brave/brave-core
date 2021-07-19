/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/crypto_dot_com/browser/crypto_dot_com_json_parser.h"

#include <map>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

double CalculateAssetVolume(double v, double h, double l) {
  // Volume is v * ((h + l) / 2)
  return v * ((h + l) / 2.0);
}

}  // namespace

bool CryptoDotComJSONParser::GetTickerInfoFromJSON(
    const std::string& json,
    CryptoDotComTickerInfo* info) {
  if (!info) {
    return false;
  }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;

  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::Value* data = records_v->FindPath("response.result.data");
  if (!data || !data->is_dict()) {
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
    return false;
  }

  const double volume =
      CalculateAssetVolume(v->GetDouble(), h->GetDouble(), l->GetDouble());

  info->insert({"volume", volume});
  info->insert({"price", price->GetDouble()});

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
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;

  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::Value* data_arr = records_v->FindPath("response.result.data");
  if (!data_arr || !data_arr->is_list()) {
    return false;
  }

  for (const base::Value &point : data_arr->GetList()) {
    std::map<std::string, double> data_point;
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
      data->clear();
      return false;
    }

    data_point.insert({"t", t->GetDouble()});
    data_point.insert({"o", o->GetDouble()});
    data_point.insert({"h", h->GetDouble()});
    data_point.insert({"l", l->GetDouble()});
    data_point.insert({"c", c->GetDouble()});
    data_point.insert({"v", v->GetDouble()});

    data->push_back(data_point);
  }

  return true;
}

bool CryptoDotComJSONParser::GetPairsFromJSON(
    const std::string& json,
    CryptoDotComSupportedPairs* pairs) {
  if (!pairs) {
    return false;
  }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;

  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::Value* instruments =
      records_v->FindPath("response.result.instruments");
  if (!instruments || !instruments->is_list()) {
    return false;
  }

  for (const base::Value &instrument : instruments->GetList()) {
    std::map<std::string, std::string> instrument_data;
    const base::Value* pair = instrument.FindKey("instrument_name");
    const base::Value* quote = instrument.FindKey("quote_currency");
    const base::Value* base = instrument.FindKey("base_currency");
    const base::Value* price = instrument.FindKey("price_decimals");
    const base::Value* quantity = instrument.FindKey("quantity_decimals");

    if (!(pair && pair->is_string()) || !(quote && quote->is_string()) ||
        !(base && base->is_string()) || !(price && price->is_int()) ||
        !(quantity && quantity->is_int())) {
      pairs->clear();
      return false;
    }

    instrument_data.insert({"pair", pair->GetString()});
    instrument_data.insert({"quote", quote->GetString()});
    instrument_data.insert({"base", base->GetString()});
    instrument_data.insert({"price", std::to_string(price->GetInt())});
    instrument_data.insert({"quantity", std::to_string(quantity->GetInt())});

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
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;

  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::Value* result = records_v->FindPath("response.result");
  if (!result) {
    return false;
  }

  // Both gainers and losers are part of the "gainers" list
  const base::Value* rankings_list = result->FindKey("gainers");
  if (!rankings_list || !rankings_list->is_list()) {
    // Gainers and losers should return empty on a bad response
    return false;
  }

  std::vector<std::map<std::string, std::string>> gainers;
  std::vector<std::map<std::string, std::string>> losers;
  for (const base::Value &ranking : rankings_list->GetList()) {
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

base::Value CryptoDotComJSONParser::GetValidAccountBalances(
    const std::string& json) {
  auto response_value = base::JSONReader::Read(json);
  if (!response_value.has_value() || !response_value.value().is_dict()) {
    return base::Value();
  }

  // Valid response has "0" for "code" property.
  if (const auto* code = response_value->FindStringKey("code")) {
    if (!code || *code != "0")
      return base::Value();
  }

  const base::Value* result_value = response_value->FindKey("result");
  if (!result_value || !result_value->is_dict()) {
    return base::Value();
  }

  base::Value valid_balances(base::Value::Type::DICTIONARY);
  const std::string* total_balance =
      result_value->FindStringKey("total_balance");
  if (!total_balance)
    return base::Value();

  const base::Value* accounts = result_value->FindListKey("accounts");
  if (!accounts)
    return base::Value();

  base::Value accounts_list(base::Value::Type::LIST);
  for (const base::Value& account : accounts->GetList()) {
    if (account.FindStringKey("stake") && account.FindStringKey("balance") &&
        account.FindStringKey("available") &&
        account.FindStringKey("currency") && account.FindStringKey("order") &&
        account.FindIntKey("currency_decimals")) {
      accounts_list.Append(account.Clone());
    }
  }

  if (accounts_list.GetList().empty())
    return base::Value();

  valid_balances.SetStringKey("total_balance", *total_balance);
  valid_balances.SetKey("accounts", std::move(accounts_list));
  return valid_balances;
}

base::Value CryptoDotComJSONParser::GetValidNewsEvents(
    const std::string& json) {
  auto response_value = base::JSONReader::Read(json);
  if (!response_value.has_value() || !response_value.value().is_dict()) {
    return base::Value();
  }

  if (const auto* code = response_value->FindStringKey("code")) {
    if (!code || *code != "0")
      return base::Value();
  }

  const base::Value* events = response_value->FindListPath("result.events");
  if (!events)
    return base::Value();

  base::Value valid_events(base::Value::Type::LIST);
  for (const base::Value& event : events->GetList()) {
    const std::string* content = event.FindStringKey("content");
    const std::string* redirect_url = event.FindStringKey("redirect_url");
    const std::string* updated_at = event.FindStringKey("updated_at");
    const std::string* redirect_title = event.FindStringKey("redirect_title");
    if (content && redirect_url && updated_at && redirect_title) {
      base::Value valid_event(base::Value::Type::DICTIONARY);
      valid_event.SetStringKey("content", *content);
      valid_event.SetStringKey("redirect_title", *redirect_title);
      valid_event.SetStringKey("redirect_url", *redirect_url);
      valid_event.SetStringKey("updated_at", *updated_at);
      valid_events.Append(std::move(valid_event));
    }
  }

  if (valid_events.GetList().empty())
    return base::Value();

  return valid_events;
}

// TODO(simonhong): Re-check return type from crypto.com service.
// Current return type is different with their spec.
base::Value CryptoDotComJSONParser::GetValidDepositAddress(
    const std::string& json) {
  auto response_value = base::JSONReader::Read(json);
  if (!response_value.has_value() || !response_value.value().is_dict()) {
    return base::Value();
  }

  // Valid response has "0" for "code" property.
  if (const auto* code = response_value->FindStringKey("code")) {
    if (*code != "0")
      return base::Value();
  }

  const base::Value* addresses_value =
      response_value->FindPath("result.addresses");
  if (!addresses_value || !addresses_value->is_list() ||
      !addresses_value->GetList().size()) {
    return base::Value();
  }

  const std::string* address_str =
      addresses_value->GetList()[0].FindStringKey("address");
  const std::string* qr_code_str =
      addresses_value->GetList()[0].FindStringKey("qr_code");
  const std::string* currency_str =
      addresses_value->GetList()[0].FindStringKey("currency");
  if (!address_str || !qr_code_str || !currency_str) {
    return base::Value();
  }

  base::Value address(base::Value::Type::DICTIONARY);
  address.SetStringKey("address", *address_str);
  address.SetStringKey("qr_code", *qr_code_str);
  address.SetStringKey("currency", *currency_str);
  return address;
}

base::Value CryptoDotComJSONParser::GetValidOrderResult(
    const std::string& json) {
  auto response_value = base::JSONReader::Read(json);
  base::Value result(base::Value::Type::DICTIONARY);
  if (!response_value.has_value() || !response_value.value().is_dict()) {
    result.SetBoolKey("success", false);
    result.SetStringKey("message",
                        l10n_util::GetStringUTF8(
                            IDS_CRYPTO_DOT_COM_WIDGET_ORDER_ERROR_MESSAGE));
    return result;
  }

  if (const auto* code = response_value->FindStringPath("result.order_id")) {
    result.SetBoolKey("success", true);
    result.SetStringKey("message", "");
    return result;
  }

  result.SetBoolKey("success", false);
  result.SetStringKey("message", "");
  if (auto* message_str = response_value->FindStringKey("result")) {
    result.SetStringKey("message", *message_str);
  } else {
    result.SetStringKey("message",
                        l10n_util::GetStringUTF8(
                            IDS_CRYPTO_DOT_COM_WIDGET_ORDER_ERROR_MESSAGE));
  }
  return result;
}
