/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENTS_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENTS_DATABASE_TABLE_H_

#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/functional/callback.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads::database::table {

using IsFirstTimeCallback =
    base::OnceCallback<void(bool success, bool is_first_time)>;

using GetAdEventsCallback =
    base::OnceCallback<void(bool success, const AdEventList& ad_events)>;
using GetAdEventVirtualPrefsCallback =
    base::OnceCallback<void(base::DictValue virtual_prefs)>;

class AdEvents final : public TableInterface {
 public:
  void RecordEvent(const AdEventInfo& ad_event, ResultCallback callback);

  // Should be called after recording the ad event. The callback takes two
  // arguments - `success` is set to `true` if successful otherwise `false`.
  // `is_first_time` is set to `true` if the ad event has only one entry
  // otherwise `false`.
  void IsFirstTime(const std::string& campaign_id,
                   mojom::ConfirmationType confirmation_type,
                   IsFirstTimeCallback callback) const;

  void GetAll(GetAdEventsCallback callback) const;

  void Get(mojom::AdType mojom_ad_type,
           mojom::ConfirmationType mojom_confirmation_type,
           base::TimeDelta time_window,
           GetAdEventsCallback callback) const;
  void GetVirtualPrefs(const base::flat_set<std::string>& ids,
                       GetAdEventVirtualPrefsCallback callback) const;

  void GetUnexpired(GetAdEventsCallback callback) const;
  void GetUnexpired(mojom::AdType mojom_ad_type,
                    GetAdEventsCallback callback) const;

  void PurgeExpired(ResultCallback callback) const;

  void PurgeForAdType(mojom::AdType ad_type, ResultCallback callback) const;

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
