/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_AD_EVENTS_DATABASE_TABLE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_AD_EVENTS_DATABASE_TABLE_H_

#include <string>

#include "base/functional/callback.h"
#include "bat/ads/ads_client_callback.h"
#include "bat/ads/internal/ads/ad_events/ad_event_info.h"
#include "bat/ads/internal/database/database_table_interface.h"
#include "bat/ads/public/interfaces/ads.mojom-forward.h"

namespace ads::database::table {

using GetAdEventsCallback =
    base::OnceCallback<void(const bool, const AdEventList&)>;

class AdEvents final : public TableInterface {
 public:
  void LogEvent(const AdEventInfo& ad_event, ResultCallback callback);

  void GetIf(const std::string& condition, GetAdEventsCallback callback) const;

  void GetAll(GetAdEventsCallback callback) const;

  void GetForType(mojom::AdType ad_type, GetAdEventsCallback callback) const;

  void PurgeExpired(ResultCallback callback) const;
  void PurgeOrphaned(mojom::AdType ad_type, ResultCallback callback) const;

  std::string GetTableName() const override;

  void Migrate(mojom::DBTransactionInfo* transaction, int to_version) override;

 private:
  void InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                      const AdEventList& ad_event);

  std::string BuildInsertOrUpdateQuery(mojom::DBCommandInfo* command,
                                       const AdEventList& ad_events) const;
};

}  // namespace ads::database::table

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_AD_EVENTS_DATABASE_TABLE_H_
