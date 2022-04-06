/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEDERATED_LOG_ENTRIES_AD_NOTIFICATION_NUMBER_OF_TABS_OPENED_IN_PAST_30_MINUTES_LOG_ENTRY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEDERATED_LOG_ENTRIES_AD_NOTIFICATION_NUMBER_OF_TABS_OPENED_IN_PAST_30_MINUTES_LOG_ENTRY_H_

#include <string>

#include "bat/ads/internal/federated/covariate_log_entry.h"

namespace ads {

class AdNotificationNumberOfTabsOpenedInPast30Minutes final
    : public CovariateLogEntry {
 public:
  AdNotificationNumberOfTabsOpenedInPast30Minutes();
  AdNotificationNumberOfTabsOpenedInPast30Minutes(
      const AdNotificationNumberOfTabsOpenedInPast30Minutes&) = delete;
  AdNotificationNumberOfTabsOpenedInPast30Minutes& operator=(
      const AdNotificationNumberOfTabsOpenedInPast30Minutes&) = delete;
  ~AdNotificationNumberOfTabsOpenedInPast30Minutes() override;

  // CovariateLogEntry
  brave_federated::mojom::DataType GetDataType() const override;
  brave_federated::mojom::CovariateType GetCovariateType() const override;
  std::string GetValue() const override;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEDERATED_LOG_ENTRIES_AD_NOTIFICATION_NUMBER_OF_TABS_OPENED_IN_PAST_30_MINUTES_LOG_ENTRY_H_
