/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_AD_EVENTS_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_AD_EVENTS_DATABASE_TABLE_H_

#include <string>

#include "base/functional/callback.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/ads_client_callback.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"

namespace brave_ads::database::table {

using GetAdEventsCallback =
    base::OnceCallback<void(bool success, const AdEventList& ad_events)>;

class AdEvents final : public TableInterface {
 public:
  void LogEvent(const AdEventInfo& ad_event, ResultCallback callback);

  void GetAll(GetAdEventsCallback callback) const;

  void GetForType(mojom::AdType ad_type, GetAdEventsCallback callback) const;

  void PurgeExpired(ResultCallback callback) const;
  void PurgeOrphaned(mojom::AdType ad_type, ResultCallback callback) const;

  std::string GetTableName() const override;

  void Create(mojom::DBTransactionInfo* transaction) override;
  void Migrate(mojom::DBTransactionInfo* transaction, int to_version) override;

 private:
  void InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                      const AdEventList& ad_event);

  std::string BuildInsertOrUpdateSql(mojom::DBCommandInfo* command,
                                     const AdEventList& ad_events) const;
};

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_AD_EVENTS_DATABASE_TABLE_H_
