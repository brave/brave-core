/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_AD_EVENTS_DATABASE_TABLE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_AD_EVENTS_DATABASE_TABLE_H_

#include <string>

#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/database/database_table.h"
#include "bat/ads/mojom.h"
#include "bat/ads/result.h"

namespace ads {

using GetAdEventsCallback =
    std::function<void(const Result, const AdEventList&)>;

namespace database {
namespace table {

class AdEvents : public Table {
 public:
  AdEvents();

  ~AdEvents() override;

  void LogEvent(const AdEventInfo& ad_event, ResultCallback callback);

  void GetIf(const std::string& condition, GetAdEventsCallback callback);

  void GetAll(GetAdEventsCallback callback);

  void PurgeExpired(ResultCallback callback);

  std::string get_table_name() const override;

  void Migrate(DBTransaction* transaction, const int to_version) override;

 private:
  void RunTransaction(const std::string& query, GetAdEventsCallback callback);

  void InsertOrUpdate(DBTransaction* transaction, const AdEventList& ad_event);

  int BindParameters(DBCommand* command, const AdEventList& ad_events);

  std::string BuildInsertOrUpdateQuery(DBCommand* command,
                                       const AdEventList& ad_events);

  void OnGetAdEvents(DBCommandResponsePtr response,
                     GetAdEventsCallback callback);

  AdEventInfo GetFromRecord(DBRecord* record) const;

  void CreateTableV5(DBTransaction* transaction);
  void MigrateToV5(DBTransaction* transaction);

  void CreateTableV13(DBTransaction* transaction);
  void MigrateToV13(DBTransaction* transaction);
};

}  // namespace table
}  // namespace database
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_AD_EVENTS_DATABASE_TABLE_H_
