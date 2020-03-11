/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <string>

#ifndef BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_ANON_FUNDS_H_
#define BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_ANON_FUNDS_H_

#include "bat/ledger/internal/contribution/contribution.h"
#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_contribution {

class ContributionAnonCard {
 public:
  explicit ContributionAnonCard(
      bat_ledger::LedgerImpl* ledger,
      Contribution* contribution);

  ~ContributionAnonCard();

  void SendTransaction(
      const double amount,
      const std::string& order_id,
      const std::string& destination,
      ledger::TransactionCallback callback);

 private:
  void OnSendTransaction(
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    ledger::TransactionCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  Contribution* contribution_;   // NOT OWNED
};

}  // namespace braveledger_contribution
#endif  // BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_ANON_FUNDS_H_
