/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_CONTRIBUTION_MONTHLY_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_CONTRIBUTION_MONTHLY_H_

#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace contribution {

class ContributionMonthly {
 public:
  explicit ContributionMonthly(LedgerImpl* ledger);

  ~ContributionMonthly();

  void Process(ledger::LegacyResultCallback callback);

  void HasSufficientBalance(
      ledger::HasSufficientBalanceToReconcileCallback callback);

 private:
  void PrepareTipList(type::PublisherInfoList list,
                      ledger::LegacyResultCallback callback);

  void GetVerifiedTipList(
      const type::PublisherInfoList& list,
      type::PublisherInfoList* verified_list);

  void OnSavePendingContribution(const type::Result result);

  void OnSufficientBalanceWallet(
      ledger::HasSufficientBalanceToReconcileCallback callback,
      const type::Result result,
      type::BalancePtr info);

  void OnHasSufficientBalance(
      const type::PublisherInfoList& publisher_list,
      const double balance,
      ledger::HasSufficientBalanceToReconcileCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace contribution
}  // namespace ledger
#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_CONTRIBUTION_MONTHLY_H_
