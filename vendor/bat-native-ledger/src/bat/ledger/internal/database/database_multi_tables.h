/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_MULTI_TABLES_H_
#define BRAVELEDGER_DATABASE_DATABASE_MULTI_TABLES_H_

#include <string>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_database {

class DatabaseMultiTables {
 public:
  explicit DatabaseMultiTables(bat_ledger::LedgerImpl* ledger);
  ~DatabaseMultiTables();

  void GetTransactionReport(
      const ledger::ActivityMonth month,
      const int year,
      ledger::GetTransactionReportCallback callback);

 private:
  void OnGetTransactionReportPromotion(
      ledger::PromotionMap promotions,
      const ledger::ActivityMonth month,
      const int year,
      ledger::GetTransactionReportCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_database

#endif  // BRAVELEDGER_DATABASE_DATABASE_MULTI_TABLES_H_
