/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_requests.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/fil_transaction.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "brave/components/json/json_helper.h"

namespace brave_wallet::fil {

std::string getBalance(const std::string& address) {
  return GetJsonRpcString("Filecoin.WalletBalance", address);
}

std::string getTransactionCount(const std::string& address) {
  return GetJsonRpcString("Filecoin.MpoolGetNonce", address);
}

std::string getEstimateGas(const std::string& from_address,
                           const std::string& to_address,
                           const std::string& gas_premium,
                           const std::string& gas_fee_cap,
                           int64_t gas_limit,
                           uint64_t nonce,
                           const std::string& max_fee,
                           const std::string& value) {
  base::Value::List params;

  base::Value::Dict transaction;
  transaction.Set("To", to_address);
  transaction.Set("From", from_address);
  transaction.Set("Value", value);
  transaction.Set("GasPremium", gas_premium.empty() ? "0" : gas_premium);
  transaction.Set("GasFeeCap", gas_fee_cap.empty() ? "0" : gas_fee_cap);

  auto parsed_addr = brave_wallet::FilAddress::FromAddress(to_address);
  if (parsed_addr.protocol() == mojom::FilecoinAddressProtocol::DELEGATED) {
    // https://github.com/filecoin-project/FIPs/blob/master/FIPS/fip-0054.md#invokecontract-method-number-38444508371
    transaction.Set("Method", "3844450837");
  } else {
    transaction.Set("Method", "0");
  }

  transaction.Set("Version", 0);
  transaction.Set("Params", "");
  transaction.Set("GasLimit", std::to_string(gas_limit));
  transaction.Set("Nonce", std::to_string(nonce));
  params.Append(std::move(transaction));

  base::Value::Dict fee;
  fee.Set("MaxFee", max_fee.empty() ? "0" : max_fee);
  params.Append(std::move(fee));

  base::Value::List cids;
  params.Append(std::move(cids));

  // TODO(cdesouza): This dictionary should be composed by GetJsonRpcDictionary.
  base::Value::Dict dict;
  dict.Set("jsonrpc", "2.0");
  dict.Set("method", "Filecoin.GasEstimateMessageGas");
  dict.Set("params", std::move(params));
  dict.Set("id", 1);
  std::string json;
  base::JSONWriter::Write(dict, &json);
  json = json::convert_string_value_to_int64("/params/0/GasLimit", json, false);
  json = json::convert_string_value_to_uint64("/params/0/Nonce", json, false);
  return json::convert_string_value_to_uint64("/params/0/Method", json, false);
}

std::string getChainHead() {
  return GetJsonRpcString("Filecoin.ChainHead");
}

std::string getStateSearchMsgLimited(const std::string& cid, uint64_t period) {
  base::Value::Dict cid_value;
  cid_value.Set("/", cid);
  auto result =
      GetJsonRpcString("Filecoin.StateSearchMsgLimited", std::move(cid_value),
                       base::Value(std::to_string(period)));
  result = json::convert_string_value_to_uint64("/params/1", result, false);
  return result;
}

std::optional<std::string> getSendTransaction(const std::string& signed_tx) {
  base::Value::List params;
  auto signed_tx_value = FilTransaction::DeserializeSignedTx(signed_tx);
  if (!signed_tx_value || !signed_tx_value->is_dict()) {
    return std::nullopt;
  }
  params.Append(std::move(signed_tx_value.value()));

  // TODO(cdesouza): This dictionary should be composed by GetJsonRpcDictionary.
  base::Value::Dict dict;
  dict.Set("jsonrpc", "2.0");
  dict.Set("method", "Filecoin.MpoolPush");
  dict.Set("params", std::move(params));
  dict.Set("id", 1);

  auto json = GetJSON(dict);
  // Use substring replace instead of deserializing signed_tx to prevent
  // long to double convertion.
  return FilTransaction::ConvertSignedTxStringFieldsToInt64("/params/0", json);
}

}  // namespace brave_wallet::fil
