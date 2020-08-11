/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_WALLET_WALLET_CLAIM_H_
#define BRAVELEDGER_WALLET_WALLET_CLAIM_H_

#include <memory>
#include <string>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/uphold/uphold.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_wallet {

class WalletClaim {
 public:
  explicit WalletClaim(bat_ledger::LedgerImpl* ledger);
  ~WalletClaim();

  void Start(ledger::ResultCallback callback);

 private:
  void OnBalance(
      const ledger::Result result,
      ledger::BalancePtr properties,
      ledger::ResultCallback callback);

  void TransferFunds(
      const std::string user_funds,
      ledger::ResultCallback callback);

  void OnTransferFunds(
      const ledger::UrlResponse& response,
      ledger::ResultCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<braveledger_uphold::Uphold> uphold_;
};

}  // namespace braveledger_wallet
#endif  // BRAVELEDGER_WALLET_WALLET_CLAIM_H_
