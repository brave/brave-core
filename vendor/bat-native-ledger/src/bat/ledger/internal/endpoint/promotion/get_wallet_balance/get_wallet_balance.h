/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ENDPOINT_PROMOTION_GET_WALLET_BALANCE_GET_WALLET_BALANCE_H_
#define BRAVELEDGER_ENDPOINT_PROMOTION_GET_WALLET_BALANCE_GET_WALLET_BALANCE_H_

#include <string>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace ledger {
namespace endpoint {
namespace promotion {

using GetWalletBalanceCallback = std::function<void(
    const ledger::Result result,
    ledger::BalancePtr balance)>;

class GetWalletBalance {
 public:
  explicit GetWalletBalance(bat_ledger::LedgerImpl* ledger);
  ~GetWalletBalance();

  void Request(GetWalletBalanceCallback callback);

 private:
  std::string GetUrl();

  ledger::Result CheckStatusCode(const int status_code);

  ledger::Result ParseBody(
      const std::string& body,
      ledger::Balance* balance);

  void OnRequest(
      const ledger::UrlResponse& response,
      GetWalletBalanceCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_PROMOTION_GET_WALLET_BALANCE_GET_WALLET_\
// BALANCE_H_
