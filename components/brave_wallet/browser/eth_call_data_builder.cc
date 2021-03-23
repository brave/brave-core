/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_call_data_builder.h"

#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"

namespace brave_wallet {

namespace erc20 {

bool BalanceOf(const std::string& address, std::string* data) {
  const std::string function_hash = GetFunctionHash("balanceOf(address)");
  std::string params;
  if (!brave_wallet::PadHexEncodedParameter(address, &params)) {
    return false;
  }
  return brave_wallet::ConcatHexStrings(function_hash, params, data);
}

}  // namespace erc20

}  // namespace brave_wallet
