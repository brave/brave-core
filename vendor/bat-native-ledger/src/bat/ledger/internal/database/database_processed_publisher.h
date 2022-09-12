/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_PROCESSED_PUBLISHER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_PROCESSED_PUBLISHER_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/database/database_table.h"

namespace ledger {
namespace database {

class DatabaseProcessedPublisher : public DatabaseTable {
 public:
  explicit DatabaseProcessedPublisher(LedgerImpl* ledger);
  ~DatabaseProcessedPublisher() override;

  void InsertOrUpdateList(const std::vector<std::string>& list,
                          ledger::LegacyResultCallback callback);

  void WasProcessed(const std::string& publisher_key,
                    ledger::LegacyResultCallback callback);

 private:
  void OnWasProcessed(mojom::DBCommandResponsePtr response,
                      ledger::LegacyResultCallback callback);
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_PROCESSED_PUBLISHER_H_
