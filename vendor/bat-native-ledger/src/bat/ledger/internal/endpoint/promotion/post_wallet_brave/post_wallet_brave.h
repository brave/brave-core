/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ENDPOINT_PROMOTION_POST_WALLET_BRAVE_POST_WALLET_BRAVE_H_
#define BRAVELEDGER_ENDPOINT_PROMOTION_POST_WALLET_BRAVE_POST_WALLET_BRAVE_H_

#include <string>

#include "bat/ledger/ledger.h"

// POST /v3/wallet/brave
//
// Request body:
// {Empty}
//
// Success code:
// HTTP_CREATED (201)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_SERVICE_UNAVAILABLE (503)
//
// Response body:
// {
//  "paymentId": "37742974-3b80-461a-acfb-937e105e5af4"
// }

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace promotion {

using PostWalletBraveCallback = std::function<void(
    const type::Result result,
    const std::string& payment_id)>;

class PostWalletBrave {
 public:
  explicit PostWalletBrave(LedgerImpl* ledger);
  ~PostWalletBrave();

  void Request(PostWalletBraveCallback callback);

 private:
  std::string GetUrl();

  type::Result CheckStatusCode(const int status_code);

  type::Result ParseBody(
      const std::string& body,
      std::string* payment_id);

  void OnRequest(
      const type::UrlResponse& response,
      PostWalletBraveCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_PROMOTION_POST_WALLET_BRAVE_POST_WALLET_BRAVE_H_
