/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_UPHOLD_UPHOLD_CONTRIBUTION_H_
#define BRAVELEDGER_UPHOLD_UPHOLD_CONTRIBUTION_H_

#include <map>
#include <string>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/uphold/uphold.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_uphold {

class UpholdContribution {
 public:
  explicit UpholdContribution(bat_ledger::LedgerImpl* ledger, Uphold* uphold);

  ~UpholdContribution();
  void Start(const std::string &viewing_id, ledger::ExternalWallet wallet);

 private:
  void CreateTransaction(double amount,
                         const std::string& address);

  void OnCreateTransaction(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers);

  void CommitTransaction(const std::string& transaction_id);

  void OnCommitTransaction(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers);

  std::string ConvertToProbi(const std::string& amount);

  void Complete(ledger::Result result);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  Uphold* uphold_;   // NOT OWNED
  std::string viewing_id_;
  ledger::ExternalWallet wallet_;
};

}  // namespace braveledger_uphold
#endif  // BRAVELEDGER_UPHOLD_UPHOLD_CONTRIBUTION_H_
