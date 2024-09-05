/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENTS_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENTS_DATABASE_TABLE_H_

#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_callback.h"

namespace brave_ads::database::table {

using GetAdEventsCallback =
    base::OnceCallback<void(bool success, const AdEventList& ad_events)>;

class AdEvents final : public TableInterface {
 public:
  void RecordEvent(const AdEventInfo& ad_event, ResultCallback callback);

  void GetAll(GetAdEventsCallback callback) const;
  // Get unexpired ad events, sorted in descending order.
  void GetUnexpired(GetAdEventsCallback callback) const;
  void GetUnexpired(mojom::AdType mojom_ad_type,
                    GetAdEventsCallback callback) const;

  void PurgeExpired(ResultCallback callback) const;

  void PurgeOrphaned(mojom::AdType mojom_ad_type,
                     ResultCallback callback) const;
  void PurgeOrphaned(const std::vector<std::string>& placement_ids,
                     ResultCallback callback) const;
  void PurgeAllOrphaned(ResultCallback callback) const;

  std::string GetTableName() const override;
  void Create(const mojom::DBTransactionInfoPtr& mojom_db_transaction) override;
  void Migrate(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
               int to_version) override;

 private:
  void Insert(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
              const AdEventList& ad_event);

  std::string BuildInsertSql(const mojom::DBActionInfoPtr& mojom_db_action,
                             const AdEventList& ad_events) const;
};

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENTS_DATABASE_TABLE_H_
