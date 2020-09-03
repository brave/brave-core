/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE__DATABASE_SERVER_PUBLISHER_BANNER_H_
#define BRAVELEDGER_DATABASE__DATABASE_SERVER_PUBLISHER_BANNER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/internal/database/database_server_publisher_amounts.h"
#include "bat/ledger/internal/database/database_server_publisher_links.h"
#include "bat/ledger/internal/database/database_table.h"

namespace ledger {
namespace database {

class DatabaseServerPublisherBanner: public DatabaseTable {
 public:
  explicit DatabaseServerPublisherBanner(LedgerImpl* ledger);
  ~DatabaseServerPublisherBanner() override;

  void InsertOrUpdate(
      type::DBTransaction* transaction,
      const type::ServerPublisherInfo& server_info);

  void DeleteRecords(
      type::DBTransaction* transaction,
      const std::string& publisher_key_list);

  void GetRecord(
      const std::string& publisher_key,
      ledger::PublisherBannerCallback callback);

 private:
  void OnGetRecord(
      type::DBCommandResponsePtr response,
      const std::string& publisher_key,
      ledger::PublisherBannerCallback callback);

  void OnGetRecordLinks(
      const std::map<std::string, std::string>& links,
      const type::PublisherBanner& banner,
      ledger::PublisherBannerCallback callback);

  void OnGetRecordAmounts(
      const std::vector<double>& amounts,
      const type::PublisherBanner& banner,
      ledger::PublisherBannerCallback callback);

  std::unique_ptr<DatabaseServerPublisherLinks> links_;
  std::unique_ptr<DatabaseServerPublisherAmounts> amounts_;
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVELEDGER_DATABASE__DATABASE_SERVER_PUBLISHER_BANNER_H_
