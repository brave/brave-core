/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/eth_request_helper.h"

#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"

namespace {

absl::optional<base::Value> GetObjectFromParamsList(const std::string& json) {
  auto json_value = base::JSONReader::Read(json);
  if (!json_value || !json_value->is_dict())
    return absl::nullopt;

  const base::Value* params = json_value->FindListPath(brave_wallet::kParams);
  if (!params || !params->is_list())
    return absl::nullopt;
  const auto list = params->GetList();
  if (list.size() != 1 || !list.front().is_dict())
    return absl::nullopt;

  return list.front().Clone();
}

// This is a best effort parsing of the data
brave_wallet::mojom::TxDataPtr ValueToTxData(const base::Value& tx_value,
                                             std::string* from_out) {
  CHECK(from_out);
  auto tx_data = brave_wallet::mojom::TxData::New();
  const base::DictionaryValue* params_dict = nullptr;
  if (!tx_value.GetAsDictionary(&params_dict) || !params_dict)
    return nullptr;

  const std::string* from = params_dict->FindStringKey("from");
  if (from)
    *from_out = *from;

  const std::string* to = params_dict->FindStringKey("to");
  if (to)
    tx_data->to = *to;

  const std::string* gas = params_dict->FindStringKey("gas");
  if (gas)
    tx_data->gas_limit = *gas;

  const std::string* gas_price = params_dict->FindStringKey("gasPrice");
  if (gas_price)
    tx_data->gas_price = *gas_price;

  const std::string* value = params_dict->FindStringKey("value");
  if (value)
    tx_data->value = *value;

  const std::string* data = params_dict->FindStringKey("data");
  if (data) {
    // If data is specified it's best to make sure it's valid
    if (data->length() <= 2 || data->substr(0, 2) != "0x")
      return nullptr;
    std::string hex_substr = data->substr(2);
    std::vector<uint8_t> bytes;
    // HexStringToBytes needs an even number of bytes
    if (hex_substr.length() % 2 == 1)
      hex_substr = "0" + hex_substr;
    if (!base::HexStringToBytes(hex_substr, &bytes))
      return nullptr;
    tx_data->data = bytes;
  }

  return tx_data;
}

// Hardcode id to 1 as it is unused
const uint32_t kRequestId = 1;
const char kRequestJsonRPC[] = "2.0";

}  // namespace

namespace brave_wallet {

mojom::TxDataPtr ParseEthSendTransactionParams(const std::string& json,
                                               std::string* from) {
  CHECK(from);
  from->clear();

  auto param_obj = GetObjectFromParamsList(json);
  if (!param_obj)
    return nullptr;
  return ValueToTxData(*param_obj, from);
}

mojom::TxData1559Ptr ParseEthSendTransaction1559Params(const std::string& json,
                                                       std::string* from) {
  CHECK(from);
  from->clear();
  auto param_obj = GetObjectFromParamsList(json);
  if (!param_obj)
    return nullptr;

  auto tx_data = mojom::TxData1559::New();
  auto base_data_ret = ValueToTxData(*param_obj, from);
  if (!base_data_ret)
    return nullptr;

  tx_data->base_data = std::move(base_data_ret);
  const base::DictionaryValue* params_dict = nullptr;
  if (!param_obj->GetAsDictionary(&params_dict) || !params_dict)
    return nullptr;

  const std::string* max_priority_fee_per_gas =
      params_dict->FindStringKey("maxPriorityFeePerGas");
  if (max_priority_fee_per_gas)
    tx_data->max_priority_fee_per_gas = *max_priority_fee_per_gas;

  const std::string* max_fee_per_gas =
      params_dict->FindStringKey("maxFeePerGas");
  if (max_fee_per_gas)
    tx_data->max_fee_per_gas = *max_fee_per_gas;

  return tx_data;
}

bool GetEthJsonRequestInfo(const std::string& json,
                           std::string* method,
                           std::string* params) {
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    return false;
  }

  const base::DictionaryValue* response_dict;
  if (!records_v->GetAsDictionary(&response_dict)) {
    return false;
  }

  if (method) {
    const std::string* found_method = response_dict->FindStringPath(kMethod);
    if (!found_method)
      return false;
    *method = *found_method;
  }

  if (params) {
    const base::Value* found_params = response_dict->FindListPath(kParams);
    if (!found_params)
      return false;
    base::JSONWriter::Write(*found_params, params);
  }

  return true;
}

bool NormalizeEthRequest(const std::string& input_json,
                         std::string* output_json) {
  CHECK(output_json);
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          input_json, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v)
    return false;

  base::DictionaryValue* out_dict;
  if (!records_v->GetAsDictionary(&out_dict))
    return false;

  ALLOW_UNUSED_LOCAL(out_dict->SetIntPath("id", kRequestId));
  ALLOW_UNUSED_LOCAL(out_dict->SetStringPath("jsonrpc", kRequestJsonRPC));
  base::JSONWriter::Write(*out_dict, output_json);
  return true;
}

}  // namespace brave_wallet
