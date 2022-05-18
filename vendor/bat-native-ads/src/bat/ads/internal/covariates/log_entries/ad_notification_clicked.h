/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COVARIATES_LOG_ENTRIES_AD_NOTIFICATION_CLICKED_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COVARIATES_LOG_ENTRIES_AD_NOTIFICATION_CLICKED_H_

#include <string>

#include "bat/ads/internal/covariates/covariate_log_entry_interface.h"

namespace ads {

class AdNotificationClicked final : public CovariateLogEntryInterface {
 public:
  AdNotificationClicked();
  AdNotificationClicked(const AdNotificationClicked&) = delete;
  AdNotificationClicked& operator=(const AdNotificationClicked&) = delete;
  ~AdNotificationClicked() override;

  void SetClicked(const bool clicked);

  // CovariateLogEntryInterface:
  brave_federated::mojom::DataType GetDataType() const override;
  brave_federated::mojom::CovariateType GetCovariateType() const override;
  std::string GetValue() const override;

 private:
  bool clicked_ = false;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COVARIATES_LOG_ENTRIES_AD_NOTIFICATION_CLICKED_H_
