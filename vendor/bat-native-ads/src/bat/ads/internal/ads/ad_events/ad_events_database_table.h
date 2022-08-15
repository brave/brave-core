/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_AD_EVENTS_DATABASE_TABLE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_AD_EVENTS_DATABASE_TABLE_H_

#include <functional>
#include <string>

#include "bat/ads/ads_client_callback.h"
#include "bat/ads/internal/ads/ad_events/ad_event_info.h"
#include "bat/ads/internal/database/database_table_interface.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {
namespace database {
namespace table {

using GetAdEventsCallback = std::function<void(const bool, const AdEventList&)>;

class AdEvents final : public TableInterface {
 public:
  AdEvents();
  ~AdEvents() override;
  AdEvents(const AdEvents&) = delete;
  AdEvents& operator=(const AdEvents&) = delete;

  void LogEvent(const AdEventInfo& ad_event, ResultCallback callback);

  void GetIf(const std::string& condition, GetAdEventsCallback callback);

  void GetAll(GetAdEventsCallback callback);

  void GetForType(const mojom::AdType ad_type, GetAdEventsCallback callback);

  void PurgeExpired(ResultCallback callback);
  void PurgeOrphaned(const mojom::AdType ad_type, ResultCallback callback);

  std::string GetTableName() const override;

  void Migrate(mojom::DBTransactionInfo* transaction,
               const int to_version) override;

 private:
  void RunTransaction(const std::string& query, GetAdEventsCallback callback);

  void InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                      const AdEventList& ad_event);

  std::string BuildInsertOrUpdateQuery(mojom::DBCommandInfo* command,
                                       const AdEventList& ad_events);

  void OnGetAdEvents(mojom::DBCommandResponseInfoPtr response,
                     GetAdEventsCallback callback);

  void MigrateToV5(mojom::DBTransactionInfo* transaction);
  void MigrateToV13(mojom::DBTransactionInfo* transaction);
  void MigrateToV17(mojom::DBTransactionInfo* transaction);
};

}  // namespace table
}  // namespace database
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_AD_EVENTS_DATABASE_TABLE_H_
