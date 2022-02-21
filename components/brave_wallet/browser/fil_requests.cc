/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_requests.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"

namespace brave_wallet {

std::string fil_getBalance(const std::string& address) {
  return GetJsonRpc1Param("Filecoin.WalletBalance", address);
}

std::string fil_getTransactionCount(const std::string& address) {
  return GetJsonRpc1Param("Filecoin.MpoolGetNonce", address);
}

std::string fil_estimateGas(const std::string& from_address,
                            const std::string& to_address,
                            const std::string& gas_premium,
                            const std::string& gas_fee_cap,
                            uint64_t gas_limit,
                            uint64_t nonce,
                            const std::string& max_fee,
                            const std::string& value) {
  base::Value params(base::Value::Type::LIST);
  base::Value transaction(base::Value::Type::DICTIONARY);
  transaction.SetKey("To", base::Value(to_address));
  transaction.SetKey("From", base::Value(from_address));
  transaction.SetKey("Value", base::Value(value));
  transaction.SetKey("GasPremium",
                     base::Value(gas_premium.empty() ? "0" : gas_premium));
  transaction.SetKey("GasFeeCap",
                     base::Value(gas_fee_cap.empty() ? "0" : gas_fee_cap));
  transaction.SetKey("Method", base::Value(0));
  transaction.SetKey("Version", base::Value(0));
  transaction.SetKey("Params", base::Value(""));
  // TODO(spylogsster): use uint64_t
  // https://github.com/filecoin-project/lotus/blob/master/chain/types/message.go#L36
  transaction.SetKey("GasLimit", base::Value(static_cast<int>(gas_limit)));
  transaction.SetKey("Nonce", base::Value(static_cast<int>(nonce)));

  params.Append(std::move(transaction));
  base::Value fee(base::Value::Type::DICTIONARY);
  fee.SetKey("MaxFee", base::Value(max_fee.empty() ? "0" : max_fee));
  params.Append(std::move(fee));
  base::Value cids(base::Value::Type::LIST);
  params.Append(std::move(cids));
  base::Value dictionary =
      GetJsonRpcDictionary("Filecoin.GasEstimateMessageGas", &params);
  return GetJSON(dictionary);
}

}  // namespace brave_wallet
