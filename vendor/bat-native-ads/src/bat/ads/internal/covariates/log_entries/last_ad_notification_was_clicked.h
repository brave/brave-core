/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COVARIATES_LOG_ENTRIES_LAST_AD_NOTIFICATION_WAS_CLICKED_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COVARIATES_LOG_ENTRIES_LAST_AD_NOTIFICATION_WAS_CLICKED_H_

#include <string>

#include "bat/ads/internal/covariates/covariate_log_entry_interface.h"

namespace ads {

class LastAdNotificationWasClicked final : public CovariateLogEntryInterface {
 public:
  LastAdNotificationWasClicked();
  LastAdNotificationWasClicked(const LastAdNotificationWasClicked&) = delete;
  LastAdNotificationWasClicked& operator=(const LastAdNotificationWasClicked&) =
      delete;
  ~LastAdNotificationWasClicked() override;

  // CovariateLogEntryInterface:
  brave_federated::mojom::DataType GetDataType() const override;
  brave_federated::mojom::CovariateType GetCovariateType() const override;
  std::string GetValue() const override;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COVARIATES_LOG_ENTRIES_LAST_AD_NOTIFICATION_WAS_CLICKED_H_
