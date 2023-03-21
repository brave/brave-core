/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COVARIATES_LOG_ENTRIES_TIME_SINCE_LAST_USER_ACTIVITY_EVENT_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COVARIATES_LOG_ENTRIES_TIME_SINCE_LAST_USER_ACTIVITY_EVENT_H_

#include <string>

#include "brave/components/brave_ads/core/internal/covariates/covariate_log_entry_interface.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_event_types.h"

namespace brave_ads {

class TimeSinceLastUserActivityEvent final : public CovariateLogEntryInterface {
 public:
  TimeSinceLastUserActivityEvent(
      UserActivityEventType event_type,
      brave_federated::mojom::CovariateType covariate_type);

  // CovariateLoCovariateLogEntryInterfacegEntry:
  brave_federated::mojom::DataType GetDataType() const override;
  brave_federated::mojom::CovariateType GetType() const override;
  std::string GetValue() const override;

 private:
  UserActivityEventType event_type_;
  brave_federated::mojom::CovariateType covariate_type_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COVARIATES_LOG_ENTRIES_TIME_SINCE_LAST_USER_ACTIVITY_EVENT_H_
