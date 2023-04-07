/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_MULTI_TABLES_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_MULTI_TABLES_H_

#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/ledger.h"

namespace ledger {
class LedgerImpl;

namespace database {

class DatabaseMultiTables {
 public:
  explicit DatabaseMultiTables(LedgerImpl* ledger);
  ~DatabaseMultiTables();

  void GetTransactionReport(const mojom::ActivityMonth month,
                            const int year,
                            ledger::GetTransactionReportCallback callback);

 private:
  void OnGetTransactionReportPromotion(
      base::flat_map<std::string, mojom::PromotionPtr> promotions,
      const mojom::ActivityMonth month,
      const int year,
      ledger::GetTransactionReportCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_MULTI_TABLES_H_
