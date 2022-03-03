/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEDERATED_LOG_ENTRIES_AD_NOTIFICATION_CLICKED_COVARIATE_LOG_ENTRY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEDERATED_LOG_ENTRIES_AD_NOTIFICATION_CLICKED_COVARIATE_LOG_ENTRY_H_

#include <string>

#include "bat/ads/internal/federated/covariate_log_entry.h"

namespace ads {

class AdNotificationClickedCovariateLogEntry final : public CovariateLogEntry {
 public:
  AdNotificationClickedCovariateLogEntry();
  AdNotificationClickedCovariateLogEntry(
      const AdNotificationClickedCovariateLogEntry&) = delete;
  AdNotificationClickedCovariateLogEntry& operator=(
      const AdNotificationClickedCovariateLogEntry&) = delete;
  ~AdNotificationClickedCovariateLogEntry() override;

  void SetClicked(const bool clicked);

  // CovariateLogEntry
  mojom::DataType GetDataType() const override;
  mojom::CovariateType GetCovariateType() const override;
  std::string GetValue() const override;

 private:
  bool was_clicked_ = false;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEDERATED_LOG_ENTRIES_AD_NOTIFICATION_CLICKED_COVARIATE_LOG_ENTRY_H_
