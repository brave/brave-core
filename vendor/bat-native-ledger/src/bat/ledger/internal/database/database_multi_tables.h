/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_MULTI_TABLES_H_
#define BRAVELEDGER_DATABASE_DATABASE_MULTI_TABLES_H_

#include <string>

#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace database {

class DatabaseMultiTables {
 public:
  explicit DatabaseMultiTables(LedgerImpl* ledger);
  ~DatabaseMultiTables();

  void GetTransactionReport(
      const type::ActivityMonth month,
      const int year,
      ledger::GetTransactionReportCallback callback);

 private:
  void OnGetTransactionReportPromotion(
      type::PromotionMap promotions,
      const type::ActivityMonth month,
      const int year,
      ledger::GetTransactionReportCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVELEDGER_DATABASE_DATABASE_MULTI_TABLES_H_
