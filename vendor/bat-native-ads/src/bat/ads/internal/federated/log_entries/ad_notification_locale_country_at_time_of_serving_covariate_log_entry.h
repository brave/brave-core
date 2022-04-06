/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEDERATED_LOG_ENTRIES_AD_NOTIFICATION_LOCALE_COUNTRY_AT_TIME_OF_SERVING_COVARIATE_LOG_ENTRY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEDERATED_LOG_ENTRIES_AD_NOTIFICATION_LOCALE_COUNTRY_AT_TIME_OF_SERVING_COVARIATE_LOG_ENTRY_H_

#include <string>

#include "bat/ads/internal/federated/covariate_log_entry.h"

namespace ads {

class AdNotificationLocaleCountryAtTimeOfServingCovariateLogEntry final
    : public CovariateLogEntry {
 public:
  AdNotificationLocaleCountryAtTimeOfServingCovariateLogEntry();
  AdNotificationLocaleCountryAtTimeOfServingCovariateLogEntry(
      const AdNotificationLocaleCountryAtTimeOfServingCovariateLogEntry&) =
      delete;
  AdNotificationLocaleCountryAtTimeOfServingCovariateLogEntry& operator=(
      const AdNotificationLocaleCountryAtTimeOfServingCovariateLogEntry&) =
      delete;
  ~AdNotificationLocaleCountryAtTimeOfServingCovariateLogEntry() override;

  // CovariateLogEntry
  brave_federated::mojom::DataType GetDataType() const override;
  brave_federated::mojom::CovariateType GetCovariateType() const override;
  std::string GetValue() const override;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEDERATED_LOG_ENTRIES_AD_NOTIFICATION_LOCALE_COUNTRY_AT_TIME_OF_SERVING_COVARIATE_LOG_ENTRY_H_
