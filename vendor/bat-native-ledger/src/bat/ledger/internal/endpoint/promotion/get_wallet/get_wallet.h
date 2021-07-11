/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_GET_WALLET_GET_WALLET_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_GET_WALLET_GET_WALLET_H_

#include <string>

#include "bat/ledger/ledger.h"

// GET /v3/wallet/{payment_id}
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
//     "paymentId": "368d87a3-7749-4ebb-9f3a-2882c99078c7",
//     "depositAccountProvider": {
//         "name": "uphold",
//         "id": "",
//         "linkingId": "4668ba96-7129-5e85-abdc-0c144ab78834"
//     },
//     "walletProvider": {
//         "id": "",
//         "name": "brave"
//     },
//     "altcurrency": "BAT",
//     "publicKey":
//     "ae55f61fa5b2870c0ee3633004c6d7a40adb5694c73d05510d8179cec8a3403a"
// }

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace promotion {

using GetWalletCallback = std::function<
    void(type::Result result, const std::string& custodian, bool linked)>;

class GetWallet {
 public:
  explicit GetWallet(LedgerImpl* ledger);
  ~GetWallet();

  void Request(GetWalletCallback callback) const;

 private:
  std::string GetUrl() const;

  void OnRequest(const type::UrlResponse& response,
                 GetWalletCallback callback) const;

  type::Result CheckStatusCode(int status_code) const;

  type::Result ParseBody(const std::string& body,
                         std::string* custodian,
                         bool* linked) const;

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_GET_WALLET_GET_WALLET_H_
