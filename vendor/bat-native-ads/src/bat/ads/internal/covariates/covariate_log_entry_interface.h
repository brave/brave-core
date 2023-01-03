/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COVARIATES_COVARIATE_LOG_ENTRY_INTERFACE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COVARIATES_COVARIATE_LOG_ENTRY_INTERFACE_H_

#include <string>

#include "brave/components/brave_federated/public/interfaces/brave_federated.mojom-shared.h"

namespace ads {

class CovariateLogEntryInterface {
 public:
  virtual ~CovariateLogEntryInterface() = default;

  virtual brave_federated::mojom::DataType GetDataType() const = 0;
  virtual brave_federated::mojom::CovariateType GetType() const = 0;
  virtual std::string GetValue() const = 0;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COVARIATES_COVARIATE_LOG_ENTRY_INTERFACE_H_
