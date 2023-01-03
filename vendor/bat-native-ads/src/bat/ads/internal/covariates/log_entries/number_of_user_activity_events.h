/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COVARIATES_LOG_ENTRIES_NUMBER_OF_USER_ACTIVITY_EVENTS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COVARIATES_LOG_ENTRIES_NUMBER_OF_USER_ACTIVITY_EVENTS_H_

#include <string>

#include "bat/ads/internal/covariates/covariate_log_entry_interface.h"
#include "bat/ads/internal/user_interaction/user_activity/user_activity_event_types.h"

namespace ads {

class NumberOfUserActivityEvents final : public CovariateLogEntryInterface {
 public:
  NumberOfUserActivityEvents(
      UserActivityEventType event_type,
      brave_federated::mojom::CovariateType covariate_type);

  // CovariateLogEntryInterface:
  brave_federated::mojom::DataType GetDataType() const override;
  brave_federated::mojom::CovariateType GetType() const override;
  std::string GetValue() const override;

 private:
  UserActivityEventType event_type_;
  brave_federated::mojom::CovariateType covariate_type_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COVARIATES_LOG_ENTRIES_NUMBER_OF_USER_ACTIVITY_EVENTS_H_
