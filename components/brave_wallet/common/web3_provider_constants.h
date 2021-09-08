/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_WEB3_PROVIDER_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_WEB3_PROVIDER_CONSTANTS_H_

#include <stdint.h>

namespace brave_wallet {

extern const char kConnectEvent[];
extern const char kDisconnectEvent[];
extern const char kChainChangedEvent[];
extern const char kAccountsChangedEvent[];

// https://eips.ethereum.org/EIPS/eip-1193#provider-errors
// https://eips.ethereum.org/EIPS/eip-1474#error-codes
enum class ProviderErrors {
  kUserRejectedRequest = 4001,  // User rejected the request
  kUnauthorized = 4100,         // The requested account and/or method has not
                                // been authorized by the user
  kUnsupportedMethod = 4200,    // The requested method is not supported by this
                                // Ethereum provider
  kDisconnected = 4900,         // The provider is disconnected from all chains
  kChainDisconnected = 4901,    // The provider is disconnected from the
                                // specified chain
  kInvalidParams = -32602,      // Invalid method parameters
  kInternalError = -32603,      // Internal JSON-RPC error
};

constexpr char kEthAccounts[] = "eth_accounts";
constexpr char kEthRequestAccounts[] = "eth_requestAccounts";
constexpr char kMethod[] = "method";
constexpr char kParams[] = "params";
constexpr char kAddEthereumChainMethod[] = "wallet_addEthereumChain";

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_WEB3_PROVIDER_CONSTANTS_H_
