/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ENDPOINT_PROMOTION_POST_WALLET_BRAVE_POST_WALLET_BRAVE_H_
#define BRAVELEDGER_ENDPOINT_PROMOTION_POST_WALLET_BRAVE_POST_WALLET_BRAVE_H_

#include <string>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace ledger {
namespace endpoint {
namespace promotion {

using PostWalletBraveCallback = std::function<void(
    const ledger::Result result,
    const std::string& payment_id)>;

class PostWalletBrave {
 public:
  explicit PostWalletBrave(bat_ledger::LedgerImpl* ledger);
  ~PostWalletBrave();

  void Request(PostWalletBraveCallback callback);

 private:
  std::string GetUrl();

  ledger::Result CheckStatusCode(const int status_code);

  ledger::Result ParseBody(
      const std::string& body,
      std::string* payment_id);

  void OnRequest(
      const ledger::UrlResponse& response,
      PostWalletBraveCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_PROMOTION_POST_WALLET_BRAVE_POST_WALLET_BRAVE_H_
