/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COVARIATES_LOG_ENTRIES_NOTIFICATION_AD_EVENT_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COVARIATES_LOG_ENTRIES_NOTIFICATION_AD_EVENT_H_

#include <string>

#include "brave/components/brave_ads/common/interfaces/ads.mojom-shared.h"
#include "brave/components/brave_ads/core/internal/covariates/covariate_log_entry_interface.h"

namespace ads {

class NotificationAdEvent final : public CovariateLogEntryInterface {
 public:
  void SetEventType(mojom::NotificationAdEventType event_type);

  // CovariateLogEntryInterface:
  brave_federated::mojom::DataType GetDataType() const override;
  brave_federated::mojom::CovariateType GetType() const override;
  std::string GetValue() const override;

 private:
  mojom::NotificationAdEventType event_type_ =
      mojom::NotificationAdEventType::kTimedOut;
};

}  // namespace ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COVARIATES_LOG_ENTRIES_NOTIFICATION_AD_EVENT_H_
