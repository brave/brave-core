/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_SERVER_PUBLISHER_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_SERVER_PUBLISHER_INFO_H_

#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/internal/database/database_server_publisher_banner.h"
#include "bat/ledger/internal/database/database_table.h"

namespace ledger {
namespace database {

class DatabaseServerPublisherInfo: public DatabaseTable {
 public:
  explicit DatabaseServerPublisherInfo(LedgerImpl* ledger);
  ~DatabaseServerPublisherInfo() override;

  void InsertOrUpdate(const mojom::ServerPublisherInfo& server_info,
                      ledger::LegacyResultCallback callback);

  void GetRecord(
      const std::string& publisher_key,
      client::GetServerPublisherInfoCallback callback);

  void DeleteExpiredRecords(int64_t max_age_seconds,
                            ledger::LegacyResultCallback callback);

 private:
  void OnGetRecordBanner(mojom::PublisherBannerPtr banner,
                         const std::string& publisher_key,
                         client::GetServerPublisherInfoCallback callback);

  void OnGetRecord(mojom::DBCommandResponsePtr response,
                   const std::string& publisher_key,
                   const mojom::PublisherBanner& banner,
                   client::GetServerPublisherInfoCallback callback);

  void OnExpiredRecordsSelected(mojom::DBCommandResponsePtr response,
                                ledger::LegacyResultCallback callback);

  std::unique_ptr<DatabaseServerPublisherBanner> banner_;
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_SERVER_PUBLISHER_INFO_H_
