/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/federated/log_entries/ad_notification_number_of_tabs_opened_in_past_30_minutes_log_entry.h"

#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/federated/covariate_logs_util.h"
#include "bat/ads/internal/user_activity/user_activity.h"
#include "bat/ads/internal/user_activity/user_activity_util.h"
#include "bat/ads/pref_names.h"

namespace ads {

AdNotificationNumberOfTabsOpenedInPast30Minutes::
    AdNotificationNumberOfTabsOpenedInPast30Minutes() = default;

AdNotificationNumberOfTabsOpenedInPast30Minutes::
    ~AdNotificationNumberOfTabsOpenedInPast30Minutes() = default;

brave_federated::mojom::DataType
AdNotificationNumberOfTabsOpenedInPast30Minutes::GetDataType() const {
  return brave_federated::mojom::DataType::kInt;
}

brave_federated::mojom::CovariateType
AdNotificationNumberOfTabsOpenedInPast30Minutes::GetCovariateType() const {
  return brave_federated::mojom::CovariateType::
      kAdNotificationNumberOfTabsOpenedInPast30Minutes;
}

std::string AdNotificationNumberOfTabsOpenedInPast30Minutes::GetValue() const {
  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(base::Minutes(30));

  return ToString(GetNumberOfTabsOpened(events));
}

}  // namespace ads
