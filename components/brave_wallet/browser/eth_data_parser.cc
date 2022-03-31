/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_data_parser.h"

#include <map>

#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

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

  if (tx_params)
    tx_params->clear();
  if (tx_args)
    tx_args->clear();

  *tx_type = mojom::TransactionType::Other;

  static std::map<std::string, mojom::TransactionType> kEthDataFunctionHashes =
      {{"0xa9059cbb", mojom::TransactionType::ERC20Transfer},
       {"0x095ea7b3", mojom::TransactionType::ERC20Approve},
       {"0x23b872dd", mojom::TransactionType::ERC721TransferFrom},
       {"0x42842e0e", mojom::TransactionType::ERC721SafeTransferFrom},
       {"0x70a08231", mojom::TransactionType::Other}};

  if (data.empty() || data == "0x0") {
    *tx_type = mojom::TransactionType::ETHSend;
    return true;
  }
  if (!IsValidHexString(data))
    return false;
  if (data.length() < 10) {
    *tx_type = mojom::TransactionType::Other;
    return true;
  }

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

    if (tx_args) {
      tx_args->push_back(address);
      tx_args->push_back(value);
    }

    if (tx_params) {
      tx_params->push_back("address");
      tx_params->push_back("uint256");
    }
  } else if (*tx_type == mojom::TransactionType::ERC721TransferFrom ||
             *tx_type == mojom::TransactionType::ERC721SafeTransferFrom) {
    std::string from, to, token_id;
    std::string left_over_data = data.substr(10);
    // Intentional copy of left_over_data
    if (!GetAddressArgFromData(std::string(left_over_data), &from,
                               &left_over_data))
      return false;
    if (!GetAddressArgFromData(std::string(left_over_data), &to,
                               &left_over_data))
      return false;
    if (!GetUint256HexFromData(std::string(left_over_data), &token_id,
                               &left_over_data))
      return false;
    // Very strictly must have correct data
    if (left_over_data.length() > 0)
      return false;

    if (tx_args) {
      tx_args->push_back(from);
      tx_args->push_back(to);
      tx_args->push_back(token_id);
    }

    if (tx_params) {
      tx_params->push_back("address");
      tx_params->push_back("address");
      tx_params->push_back("uint256");
    }
  }

  return true;
}

}  // namespace brave_wallet
