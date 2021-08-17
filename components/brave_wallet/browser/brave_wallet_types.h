/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_TYPES_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_TYPES_H_

#include <string>
#include <vector>

#include "base/callback.h"

namespace brave_wallet {

typedef unsigned _ExtInt(256) uint256_t;

struct TransactionReceipt {
  TransactionReceipt();
  ~TransactionReceipt();
  TransactionReceipt(const TransactionReceipt&);
  bool operator==(const TransactionReceipt&) const;
  bool operator!=(const TransactionReceipt&) const;

  std::string transaction_hash;
  uint256_t transaction_index;
  std::string block_hash;
  uint256_t block_number;
  std::string from;
  std::string to;
  uint256_t cumulative_gas_used;
  uint256_t gas_used;
  std::string contract_address;
  std::vector<std::string> logs;
  std::string logs_bloom;
  bool status;
};

struct NativeCurrency {
  std::string name;
  std::string symbol;
  uint256_t decimals;
};

struct EthereumChain {
  EthereumChain();
  ~EthereumChain();
  EthereumChain(const EthereumChain&);

  std::string chain_id;
  std::string chain_name;
  std::vector<std::string> block_explorer_urls;
  std::vector<std::string> icon_urls;
  std::vector<std::string> rpc_urls;
  NativeCurrency currency;
};

using RequestEthereumChainCallback =
    base::OnceCallback<void(const std::string&)>;

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_TYPES_H_
