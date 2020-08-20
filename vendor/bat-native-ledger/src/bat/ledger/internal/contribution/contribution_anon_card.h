/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_ANON_FUNDS_H_
#define BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_ANON_FUNDS_H_

#include <map>
#include <memory>
#include <string>

#include "bat/ledger/internal/endpoint/payment/payment_server.h"
#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_contribution {

class ContributionAnonCard {
 public:
  explicit ContributionAnonCard(bat_ledger::LedgerImpl* ledger);

  ~ContributionAnonCard();

  void SendTransaction(
      const double amount,
      const std::string& order_id,
      const std::string& destination,
      ledger::TransactionCallback callback);

 private:
  void OnSendTransaction(
      const ledger::Result result,
      ledger::TransactionCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<ledger::endpoint::PaymentServer> payment_server_;
};

}  // namespace braveledger_contribution
#endif  // BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_ANON_FUNDS_H_
