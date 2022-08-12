/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_GET_RECOVER_WALLET_GET_RECOVER_WALLET_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_GET_RECOVER_WALLET_GET_RECOVER_WALLET_H_

#include <string>

#include "bat/ledger/ledger.h"

// GET /v3/wallet/recover/{public_key}
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_NOT_FOUND (404)
//
// Response body:
// {
//  "paymentId": "d59d4b69-f66e-4ee8-9c88-1c522e02ffd3",
//  "walletProvider": {
//    "id": "a9d12d76-2b6d-4f8b-99df-bb801bff9407",
//    "name": "uphold"
//  },
//  "altcurrency": "BAT",
//  "publicKey": "79d7da2a756cc8d9403d0353a64fae5698e01b44a2c2745"
// }

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace promotion {

using GetRecoverWalletCallback =
    std::function<void(type::Result result, const std::string& payment_id)>;

class GetRecoverWallet {
 public:
  explicit GetRecoverWallet(LedgerImpl* ledger);
  ~GetRecoverWallet();

  void Request(
    const std::string& public_key_hex,
    GetRecoverWalletCallback callback);

 private:
  std::string GetUrl(const std::string& public_key_hex);

  type::Result CheckStatusCode(const int status_code);

  type::Result ParseBody(const std::string& body, std::string* payment_id);

  void OnRequest(
      const type::UrlResponse& response,
      GetRecoverWalletCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_GET_RECOVER_WALLET_GET_RECOVER_WALLET_H_
