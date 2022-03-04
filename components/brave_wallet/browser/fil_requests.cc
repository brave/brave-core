/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_requests.h"

#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"

namespace brave_wallet {

std::string fil_getBalance(const std::string& address) {
  return GetJsonRpc1Param("Filecoin.WalletBalance", address);
}

std::string fil_getTransactionCount(const std::string& address) {
  return GetJsonRpc1Param("Filecoin.MpoolGetNonce", address);
}

}  // namespace brave_wallet
