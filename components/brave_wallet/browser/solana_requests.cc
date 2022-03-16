/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_requests.h"

#include <utility>

#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"

namespace brave_wallet {

namespace solana {

std::string getBalance(const std::string& pubkey) {
  return GetJsonRpc1Param("getBalance", pubkey);
}

std::string getTokenAccountBalance(const std::string& pubkey) {
  return GetJsonRpc1Param("getTokenAccountBalance", pubkey);
}

std::string sendTransaction(const std::string& signed_tx) {
  base::Value params(base::Value::Type::LIST);
  params.Append(signed_tx);

  // Set encoding to base64 because the document says base58 is currently the
  // default value but is slow and deprecated.
  base::Value configuration(base::Value::Type::DICTIONARY);
  configuration.SetStringKey("encoding", "base64");
  params.Append(std::move(configuration));

  base::Value dictionary = GetJsonRpcDictionary("sendTransaction", &params);
  return GetJSON(dictionary);
}

std::string getLatestBlockhash() {
  return GetJsonRpcNoParams("getLatestBlockhash");
}

}  // namespace solana

}  // namespace brave_wallet
