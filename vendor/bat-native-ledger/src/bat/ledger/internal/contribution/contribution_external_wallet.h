/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_EXTERNAL_WALLET_H_
#define BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_EXTERNAL_WALLET_H_

#include <map>
#include <string>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/uphold/uphold.h"

namespace ledger {
class LedgerImpl;

namespace contribution {

class ContributionExternalWallet {
 public:
  explicit ContributionExternalWallet(
      LedgerImpl* ledger,
      uphold::Uphold* uphold);

  ~ContributionExternalWallet();

  void Process(
      const std::string& contribution_id,
      ledger::ResultCallback callback);

  void Retry(
      ledger::ContributionInfoPtr contribution,
      ledger::ResultCallback callback);

 private:
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
      const ledger::RewardsType type,
      const bool single_publisher,
      ledger::ResultCallback callback);

  void Completed(
      const ledger::Result result,
      const bool single_publisher,
      ledger::ResultCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
  uphold::Uphold* uphold_;  // NOT OWNED
};

}  // namespace contribution
}  // namespace ledger
#endif  // BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_EXTERNAL_WALLET_H_
