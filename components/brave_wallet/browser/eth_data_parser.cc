/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_data_parser.h"

#include <map>

#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_types.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"

namespace brave_wallet {

namespace {

bool GetAddressArgFromData(const std::string& input,
                           std::string* arg,
                           std::string* left_over) {
  CHECK(arg);
  CHECK(left_over);
  if (input.length() < 64) {
    return false;
  }
  *left_over = input.substr(64);
  *arg = "0x" + input.substr(24, 40);  // Get rid of 24 0-padded chars
  return true;
}

bool GetUint256HexFromData(const std::string& input,
                           std::string* arg,
                           std::string* left_over) {
  CHECK(arg);
  CHECK(left_over);
  if (input.length() < 64) {
    return false;
  }
  *left_over = input.substr(64);
  std::string padded_arg = "0x" + input.substr(0, 64);

  uint256_t arg_uint;
  if (!brave_wallet::HexValueToUint256(padded_arg, &arg_uint)) {
    return false;
  }
  *arg = brave_wallet::Uint256ValueToHex(arg_uint);
  return true;
}

}  // namespace

bool GetTransactionInfoFromData(const std::string& data,
                                mojom::TransactionType* tx_type,
                                std::vector<std::string>* tx_params,
                                std::vector<std::string>* tx_args) {
  CHECK(tx_type);
  CHECK(tx_params);
  CHECK(tx_args);

  tx_params->clear();
  tx_args->clear();
  *tx_type = mojom::TransactionType::Other;

  static std::map<std::string, mojom::TransactionType> kEthDataFunctionHashes =
      {{"0xa9059cbb", mojom::TransactionType::ERC20Transfer},
       {"0x095ea7b3", mojom::TransactionType::ERC20Approve},
       {"0x70a08231", mojom::TransactionType::Other}};

  if (data.empty() || data == "0x0") {
    *tx_type = mojom::TransactionType::ETHSend;
    return true;
  }
  if (!IsValidHexString(data))
    return false;
  if (data.length() < 10)
    return false;

  std::string function_hash = base::ToLowerASCII(data.substr(0, 10));
  auto it = kEthDataFunctionHashes.find(function_hash);
  if (it != kEthDataFunctionHashes.end())
    *tx_type = it->second;
  else
    *tx_type = mojom::TransactionType::Other;

  if (*tx_type == mojom::TransactionType::ERC20Transfer ||
      *tx_type == mojom::TransactionType::ERC20Approve) {
    std::string address, value;
    std::string left_over_data = data.substr(10);
    // Intentional copy of left_over_data
    if (!GetAddressArgFromData(std::string(left_over_data), &address,
                               &left_over_data))
      return false;
    if (!GetUint256HexFromData(std::string(left_over_data), &value,
                               &left_over_data))
      return false;
    // Very strictly must have correct data
    if (left_over_data.length() > 0)
      return false;
    tx_args->push_back(address);
    tx_args->push_back(value);
    tx_params->push_back("address");
    tx_params->push_back("uint256");
  }

  return true;
}

}  // namespace brave_wallet
