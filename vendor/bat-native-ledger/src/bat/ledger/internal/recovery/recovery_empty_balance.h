/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_RECOVERY_RECOVERY_EMPTY_BALANCE_H_
#define BRAVELEDGER_RECOVERY_RECOVERY_EMPTY_BALANCE_H_

#include "bat/ledger/internal/ledger_impl.h"

namespace braveledger_recovery {

class EmptyBalance {
 public:
  static void Check(bat_ledger::LedgerImpl* ledger);

 private:
  static void OnAllContributions(
      ledger::ContributionInfoList list,
      bat_ledger::LedgerImpl* ledger);

  static void GetPromotions(
      bat_ledger::LedgerImpl* ledger,
      ledger::GetPromotionListCallback callback);

  static void OnPromotions(
      ledger::PromotionMap promotions,
      ledger::GetPromotionListCallback callback);

  static void GetCredsByPromotions(
      ledger::PromotionList list,
      bat_ledger::LedgerImpl* ledger);

  static void OnCreds(
      ledger::CredsBatchList list,
      bat_ledger::LedgerImpl* ledger);

  static void OnSaveUnblindedCreds(
      const ledger::Result result,
      bat_ledger::LedgerImpl* ledger);

  static void GetAllTokens(
      ledger::PromotionList list,
      bat_ledger::LedgerImpl* ledger,
      const double contribution_sum);

  static void ReportResults(
      ledger::UnblindedTokenList list,
      bat_ledger::LedgerImpl* ledger,
      const double contribution_sum,
      const double promotion_sum);

  static void Sent(
      const ledger::UrlResponse& response,
      bat_ledger::LedgerImpl* ledger);
};

}  // namespace braveledger_recovery

#endif  // BRAVELEDGER_RECOVERY_RECOVERY_EMPTY_BALANCE_H_
