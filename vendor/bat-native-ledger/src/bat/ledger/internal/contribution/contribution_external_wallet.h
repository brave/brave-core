/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <string>

#ifndef BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_EXTERNAL_WALLET_H_
#define BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_EXTERNAL_WALLET_H_

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/contribution/contribution.h"
#include "bat/ledger/internal/uphold/uphold.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_contribution {

class ContributionExternalWallet {
 public:
  explicit ContributionExternalWallet(
      bat_ledger::LedgerImpl* ledger,
      Contribution* contribution,
      braveledger_uphold::Uphold* uphold);

  ~ContributionExternalWallet();

  void Process(
      const std::string& contribution_id,
      ledger::ResultCallback callback);

  void Retry(
      ledger::ContributionInfoPtr contribution,
      ledger::ResultCallback callback);

 private:
  void OnExternalWallets(
      std::map<std::string, ledger::ExternalWalletPtr> wallets,
      const std::string& contribution_id,
      ledger::ResultCallback callback);

  void ContributionInfo(
      ledger::ContributionInfoPtr contribution,
      const ledger::ExternalWallet& wallet,
      ledger::ResultCallback callback);

  void OnAC(
      const ledger::Result result,
      const std::string& contribution_id);

  void OnSavePendingContribution(
      const ledger::Result result);

  void OnServerPublisherInfo(
      ledger::ServerPublisherInfoPtr info,
      const std::string& contribution_id,
      const double amount,
      const ledger::ExternalWallet& wallet,
      const ledger::RewardsType type,
      const bool single_publisher,
      ledger::ResultCallback callback);

  void Completed(
      const ledger::Result result,
      const bool single_publisher,
      ledger::ResultCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  Contribution* contribution_;   // NOT OWNED
  braveledger_uphold::Uphold* uphold_;  // NOT OWNED
};

}  // namespace braveledger_contribution
#endif  // BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_EXTERNAL_WALLET_H_
