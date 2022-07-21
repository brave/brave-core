/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_PUBLISHER_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_PUBLISHER_INFO_H_

#include <string>

#include "bat/ledger/internal/database/database_table.h"

namespace ledger {
namespace database {

class DatabasePublisherInfo: public DatabaseTable {
 public:
  explicit DatabasePublisherInfo(LedgerImpl* ledger);
  ~DatabasePublisherInfo() override;

  void InsertOrUpdate(type::PublisherInfoPtr info,
                      ledger::LegacyResultCallback callback);

  void GetRecord(
      const std::string& publisher_key,
      ledger::PublisherInfoCallback callback);

  void GetPanelRecord(
      type::ActivityInfoFilterPtr filter,
      ledger::PublisherInfoCallback callback);

  void RestorePublishers(ledger::LegacyResultCallback callback);

  void GetExcludedList(ledger::PublisherInfoListCallback callback);

 private:
  void OnGetRecord(
      type::DBCommandResponsePtr response,
      ledger::PublisherInfoCallback callback);

  void OnGetPanelRecord(
      type::DBCommandResponsePtr response,
      ledger::PublisherInfoCallback callback);

  void OnGetExcludedList(
      type::DBCommandResponsePtr response,
      ledger::PublisherInfoListCallback callback);
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_PUBLISHER_INFO_H_
