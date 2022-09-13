/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_RECURRING_TIP_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_RECURRING_TIP_H_

#include <string>

#include "bat/ledger/internal/database/database_table.h"

namespace ledger {
namespace database {

class DatabaseRecurringTip: public DatabaseTable {
 public:
  explicit DatabaseRecurringTip(LedgerImpl* ledger);
  ~DatabaseRecurringTip() override;

  void InsertOrUpdate(mojom::RecurringTipPtr info,
                      ledger::LegacyResultCallback callback);

  void GetAllRecords(ledger::PublisherInfoListCallback callback);

  void DeleteRecord(const std::string& publisher_key,
                    ledger::LegacyResultCallback callback);

 private:
  void OnGetAllRecords(mojom::DBCommandResponsePtr response,
                       ledger::PublisherInfoListCallback callback);
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_RECURRING_TIP_H_
