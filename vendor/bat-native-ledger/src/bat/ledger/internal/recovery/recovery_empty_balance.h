/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_RECOVERY_RECOVERY_EMPTY_BALANCE_H_
#define BRAVELEDGER_RECOVERY_RECOVERY_EMPTY_BALANCE_H_

#include <memory>

#include "bat/ledger/internal/endpoint/promotion/promotion_server.h"

namespace ledger {
class LedgerImpl;

namespace recovery {

class EmptyBalance {
 public:
  explicit EmptyBalance(LedgerImpl* ledger);
  ~EmptyBalance();

  void Check();

 private:
  void OnAllContributions(type::ContributionInfoList list);

  void GetPromotions(client::GetPromotionListCallback callback);

  void OnPromotions(
      type::PromotionMap promotions,
      client::GetPromotionListCallback callback);

  void GetCredsByPromotions(type::PromotionList list);

  void OnCreds(type::CredsBatchList list);

  void OnSaveUnblindedCreds(const type::Result result);

  void GetAllTokens(
      type::PromotionList list,
      const double contribution_sum);

  void ReportResults(
      type::UnblindedTokenList list,
      const double contribution_sum,
      const double promotion_sum);

  void Sent(const type::Result result);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<endpoint::PromotionServer> promotion_server_;
};

}  // namespace recovery
}  // namespace ledger

#endif  // BRAVELEDGER_RECOVERY_RECOVERY_EMPTY_BALANCE_H_
