/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_WALLET_WALLET_CLAIM_H_
#define BRAVELEDGER_WALLET_WALLET_CLAIM_H_

#include <memory>
#include <string>

#include "bat/ledger/internal/endpoint/promotion/promotion_server.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace wallet {

class WalletClaim {
 public:
  explicit WalletClaim(LedgerImpl* ledger);
  ~WalletClaim();

  void Start(ledger::ResultCallback callback);

 private:
  void OnBalance(
      const type::Result result,
      type::BalancePtr properties,
      ledger::ResultCallback callback);

  void OnTransferFunds(
      const type::Result result,
      ledger::ResultCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<endpoint::PromotionServer> promotion_server_;
};

}  // namespace wallet
}  // namespace ledger
#endif  // BRAVELEDGER_WALLET_WALLET_CLAIM_H_
