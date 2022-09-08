/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_RECOVERY_RECOVERY_EMPTY_BALANCE_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_RECOVERY_RECOVERY_EMPTY_BALANCE_H_

#include <memory>
#include <string>
#include <vector>

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
  void OnAllContributions(std::vector<mojom::ContributionInfoPtr> list);

  void GetPromotions(client::GetPromotionListCallback callback);

  void OnPromotions(base::flat_map<std::string, mojom::PromotionPtr> promotions,
                    client::GetPromotionListCallback callback);

  void GetCredsByPromotions(std::vector<mojom::PromotionPtr> list);

  void OnCreds(std::vector<mojom::CredsBatchPtr> list);

  void OnSaveUnblindedCreds(const mojom::Result result);

  void GetAllTokens(std::vector<mojom::PromotionPtr> list,
                    const double contribution_sum);

  void ReportResults(std::vector<mojom::UnblindedTokenPtr> list,
                     const double contribution_sum,
                     const double promotion_sum);

  void Sent(const mojom::Result result);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<endpoint::PromotionServer> promotion_server_;
};

}  // namespace recovery
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_RECOVERY_RECOVERY_EMPTY_BALANCE_H_
