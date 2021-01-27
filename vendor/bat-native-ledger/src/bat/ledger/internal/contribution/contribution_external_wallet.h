/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_CONTRIBUTION_EXTERNAL_WALLET_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_CONTRIBUTION_EXTERNAL_WALLET_H_

#include <map>
#include <string>

#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace contribution {

class ContributionExternalWallet {
 public:
  explicit ContributionExternalWallet(LedgerImpl* ledger);

  ~ContributionExternalWallet();

  void Process(
      const std::string& contribution_id,
      ledger::ResultCallback callback);

  void Retry(
      type::ContributionInfoPtr contribution,
      ledger::ResultCallback callback);

 private:
  void ContributionInfo(
      type::ContributionInfoPtr contribution,
      ledger::ResultCallback callback);

  void OnAC(
      const type::Result result,
      const std::string& contribution_id);

  void OnSavePendingContribution(
      const type::Result result);

  void OnServerPublisherInfo(type::ServerPublisherInfoPtr info,
                             const std::string& contribution_id,
                             const double amount,
                             const type::RewardsType type,
                             const type::ContributionProcessor processor,
                             const bool single_publisher,
                             ledger::ResultCallback callback);

  void Completed(
      const type::Result result,
      const bool single_publisher,
      ledger::ResultCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace contribution
}  // namespace ledger
#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_CONTRIBUTION_EXTERNAL_WALLET_H_
