/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEDERATED_LOG_ENTRIES_AD_NOTIFICATION_IMPRESSION_SERVED_AT_COVARIATE_LOG_ENTRY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEDERATED_LOG_ENTRIES_AD_NOTIFICATION_IMPRESSION_SERVED_AT_COVARIATE_LOG_ENTRY_H_

#include <string>

#include "base/time/time.h"
#include "bat/ads/internal/federated/covariate_log_entry.h"

namespace ads {

class AdNotificationImpressionServedAtCovariateLogEntry final
    : public CovariateLogEntry {
 public:
  AdNotificationImpressionServedAtCovariateLogEntry();
  AdNotificationImpressionServedAtCovariateLogEntry(
      const AdNotificationImpressionServedAtCovariateLogEntry&) = delete;
  AdNotificationImpressionServedAtCovariateLogEntry& operator=(
      const AdNotificationImpressionServedAtCovariateLogEntry&) = delete;
  ~AdNotificationImpressionServedAtCovariateLogEntry() override;

  void SetLastImpressionAt(const base::Time time);

  // CovariateLogEntry
  brave_federated::mojom::DataType GetDataType() const override;
  brave_federated::mojom::CovariateType GetCovariateType() const override;
  std::string GetValue() const override;

 private:
  base::Time impression_served_at_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEDERATED_LOG_ENTRIES_AD_NOTIFICATION_IMPRESSION_SERVED_AT_COVARIATE_LOG_ENTRY_H_
