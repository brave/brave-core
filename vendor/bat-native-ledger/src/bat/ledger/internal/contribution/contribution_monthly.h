/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_MONTHLY_H_
#define BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_MONTHLY_H_

#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace contribution {

class ContributionMonthly {
 public:
  explicit ContributionMonthly(LedgerImpl* ledger);

  ~ContributionMonthly();

  void Process(ledger::ResultCallback callback);

  void HasSufficientBalance(
      ledger::HasSufficientBalanceToReconcileCallback callback);

 private:
  void PrepareTipList(
      type::PublisherInfoList list,
      ledger::ResultCallback callback);

  void GetVerifiedTipList(
      const type::PublisherInfoList& list,
      type::PublisherInfoList* verified_list);

  void OnSavePendingContribution(const type::Result result);

  void OnSufficientBalanceWallet(
      const type::Result result,
      type::BalancePtr info,
      ledger::HasSufficientBalanceToReconcileCallback callback);

  void OnHasSufficientBalance(
      const type::PublisherInfoList& publisher_list,
      const double balance,
      ledger::HasSufficientBalanceToReconcileCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace contribution
}  // namespace ledger
#endif  // BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_MONTHLY_H_
