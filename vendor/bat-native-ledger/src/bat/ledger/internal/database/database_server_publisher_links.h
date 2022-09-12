/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_SERVER_PUBLISHER_LINKS_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_SERVER_PUBLISHER_LINKS_H_

#include <map>
#include <string>
#include <vector>

#include "bat/ledger/internal/database/database_table.h"

namespace ledger {
namespace database {

class DatabaseServerPublisherLinks: public DatabaseTable {
 public:
  explicit DatabaseServerPublisherLinks(LedgerImpl* ledger);
  ~DatabaseServerPublisherLinks() override;

  void InsertOrUpdate(mojom::DBTransaction* transaction,
                      const mojom::ServerPublisherInfo& server_info);

  void DeleteRecords(mojom::DBTransaction* transaction,
                     const std::string& publisher_key_list);

  void GetRecord(
      const std::string& publisher_key,
      ServerPublisherLinksCallback callback);

 private:
  void OnGetRecord(mojom::DBCommandResponsePtr response,
                   ServerPublisherLinksCallback callback);
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_SERVER_PUBLISHER_LINKS_H_
