/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/web3_provider_constants.h"

namespace brave_wallet {

const char kConnectEvent[] = "connect";
const char kDisconnectEvent[] = "disconnect";
const char kChainChangedEvent[] = "chainChanged";
const char kAccountsChangedEvent[] = "accountsChanged";
const char kAddEthereumChainMethod[] = "wallet_addEthereumChain";
const char kJsonResponseAddEthereumChainSuccess[] =
    R"({"id":1,"jsonrpc":"2.0", "result": null})";
const char kJsonResponseAddEthereumChainError[] =
    R"({"id":1,"jsonrpc":"2.0", "error": { "code": 4001, "message": "%s" }})";

}  // namespace brave_wallet
