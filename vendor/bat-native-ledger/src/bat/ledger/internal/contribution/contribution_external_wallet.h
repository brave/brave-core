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

  void Process(const std::string& contribution_id);

 private:
  void OnExternalWallets(
      const std::string& contribution_id,
      std::map<std::string, ledger::ExternalWalletPtr> wallets);

  void ContributionInfo(
      ledger::ContributionInfoPtr contribution,
      const ledger::ExternalWallet& wallet);

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
      const ledger::RewardsType type);

  void Completed(
      const ledger::Result result,
      const double amount,
      const std::string& contribution_id,
      const ledger::RewardsType type);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  Contribution* contribution_;   // NOT OWNED
  braveledger_uphold::Uphold* uphold_;  // NOT OWNED
};

}  // namespace braveledger_contribution
#endif  // BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_EXTERNAL_WALLET_H_
