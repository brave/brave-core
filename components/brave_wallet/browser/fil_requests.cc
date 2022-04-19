/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_requests.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/json/rs/src/lib.rs.h"

namespace brave_wallet {

namespace fil {

std::string getBalance(const std::string& address) {
  return GetJsonRpc1Param("Filecoin.WalletBalance", address);
}

std::string getTransactionCount(const std::string& address) {
  return GetJsonRpc1Param("Filecoin.MpoolGetNonce", address);
}

std::string getEstimateGas(const std::string& from_address,
                           const std::string& to_address,
                           const std::string& gas_premium,
                           const std::string& gas_fee_cap,
                           int64_t gas_limit,
                           uint64_t nonce,
                           const std::string& max_fee,
                           const std::string& value) {
  base::Value params(base::Value::Type::LIST);

  base::Value transaction(base::Value::Type::DICTIONARY);
  transaction.SetStringKey("To", to_address);
  transaction.SetStringKey("From", from_address);
  transaction.SetStringKey("Value", value);
  transaction.SetStringKey("GasPremium",
                           gas_premium.empty() ? "0" : gas_premium);
  transaction.SetStringKey("GasFeeCap",
                           gas_fee_cap.empty() ? "0" : gas_fee_cap);
  transaction.SetIntKey("Method", 0);
  transaction.SetIntKey("Version", 0);
  transaction.SetStringKey("Params", "");
  transaction.SetStringKey("GasLimit", std::to_string(gas_limit));
  transaction.SetStringKey("Nonce", std::to_string(nonce));
  params.Append(std::move(transaction));

  base::Value fee(base::Value::Type::DICTIONARY);
  fee.SetStringKey("MaxFee", max_fee.empty() ? "0" : max_fee);
  params.Append(std::move(fee));

  base::Value cids(base::Value::Type::LIST);
  params.Append(std::move(cids));

  base::Value dictionary(base::Value::Type::DICTIONARY);
  dictionary.SetStringKey("jsonrpc", "2.0");
  dictionary.SetStringKey("method", "Filecoin.GasEstimateMessageGas");
  dictionary.SetKey("params", std::move(params));
  dictionary.SetIntKey("id", 1);
  std::string json;
  base::JSONWriter::Write(dictionary, &json);
  json = std::string(json::convert_string_value_to_int64("/params/0/GasLimit",
                                                         json.c_str(), false)
                         .c_str());
  return std::string(json::convert_string_value_to_uint64("/params/0/Nonce",
                                                          json.c_str(), false)
                         .c_str());
}

std::string getChainHead() {
  return GetJsonRpcNoParams("Filecoin.ChainHead");
}

std::string getStateSearchMsgLimited(const std::string& cid, uint64_t period) {
  base::Value cid_value(base::Value::Type::DICTIONARY);
  cid_value.SetStringKey("/", cid);
  auto result =
      GetJsonRpc2Params("Filecoin.StateSearchMsgLimited", std::move(cid_value),
                        base::Value(std::to_string(period)));
  result = std::string(
      json::convert_string_value_to_uint64("/params/1", result.c_str(), false)
          .c_str());
  return result;
}

absl::optional<std::string> getSendTransaction(const std::string& signed_tx) {
  base::JSONReader::ValueWithError parsed_tx =
      base::JSONReader::ReadAndReturnValueWithError(signed_tx);
  if (!parsed_tx.value || !parsed_tx.value->is_dict()) {
    return absl::nullopt;
  }
  base::Value params(base::Value::Type::LIST);
  params.Append(std::move(*parsed_tx.value));

  base::Value dictionary(base::Value::Type::DICTIONARY);
  dictionary.SetStringKey("jsonrpc", "2.0");
  dictionary.SetStringKey("method", "Filecoin.MpoolPush");
  dictionary.SetKey("params", std::move(params));
  dictionary.SetIntKey("id", 1);

  return GetJSON(dictionary);
}

}  // namespace fil

}  // namespace brave_wallet
