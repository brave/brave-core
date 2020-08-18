/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_RECOVERY_RECOVERY_EMPTY_BALANCE_H_
#define BRAVELEDGER_RECOVERY_RECOVERY_EMPTY_BALANCE_H_

#include <memory>

#include "bat/ledger/internal/endpoint/promotion/promotion_server.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace ledger {
namespace recovery {

class EmptyBalance {
 public:
  explicit EmptyBalance(bat_ledger::LedgerImpl* ledger);
  ~EmptyBalance();

  void Check();

 private:
  void OnAllContributions(ledger::ContributionInfoList list);

  void GetPromotions(ledger::GetPromotionListCallback callback);

  void OnPromotions(
      ledger::PromotionMap promotions,
      ledger::GetPromotionListCallback callback);

  void GetCredsByPromotions(ledger::PromotionList list);

  void OnCreds(ledger::CredsBatchList list);

  void OnSaveUnblindedCreds(const ledger::Result result);

  void GetAllTokens(
      ledger::PromotionList list,
      const double contribution_sum);

  void ReportResults(
      ledger::UnblindedTokenList list,
      const double contribution_sum,
      const double promotion_sum);

  void Sent(const ledger::Result result);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<endpoint::PromotionServer> promotion_server_;
};

}  // namespace recovery
}  // namespace ledger

#endif  // BRAVELEDGER_RECOVERY_RECOVERY_EMPTY_BALANCE_H_
