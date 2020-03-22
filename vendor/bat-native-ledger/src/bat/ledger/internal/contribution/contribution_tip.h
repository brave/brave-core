/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#ifndef BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_TIP_H_
#define BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_TIP_H_

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/contribution/contribution.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_contribution {

class ContributionTip {
 public:
  explicit ContributionTip(
      bat_ledger::LedgerImpl* ledger,
      Contribution* contribution);

  ~ContributionTip();

  void Process(
      const std::string& publisher_key,
      const double amount,
      ledger::ResultCallback callback);

 private:
  void ServerPublisher(
      ledger::ServerPublisherInfoPtr server_info,
      const std::string& publisher_key,
      const double amount,
      ledger::ResultCallback callback);

  void SavePending(
      const std::string& publisher_key,
      const double amount,
      ledger::ResultCallback callback);

  void OnSavePending(
      const ledger::Result result,
      ledger::ResultCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  Contribution* contribution_;   // NOT OWNED
};

}  // namespace braveledger_contribution
#endif  // BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_TIP_H_
