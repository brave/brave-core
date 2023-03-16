/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COVARIATES_LOG_ENTRIES_AVERAGE_CLICKTHROUGH_RATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COVARIATES_LOG_ENTRIES_AVERAGE_CLICKTHROUGH_RATE_H_

#include <string>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/covariates/covariate_log_entry_interface.h"

namespace brave_ads {

class AverageClickthroughRate final : public CovariateLogEntryInterface {
 public:
  explicit AverageClickthroughRate(base::TimeDelta time_window);

  // CovariateLogEntryInterface:
  brave_federated::mojom::DataType GetDataType() const override;
  brave_federated::mojom::CovariateType GetType() const override;
  std::string GetValue() const override;

 private:
  base::TimeDelta time_window_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COVARIATES_LOG_ENTRIES_AVERAGE_CLICKTHROUGH_RATE_H_
