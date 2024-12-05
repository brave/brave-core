/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_DATA_DATA_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_DATA_DATA_H_

#include "brave/components/brave_ads/core/internal/ml/data/data_types.h"

namespace brave_ads::ml {

class Data {
 public:
  explicit Data(DataType type);

  Data(const Data&) = delete;
  Data& operator=(const Data&) = delete;

  virtual ~Data();

  DataType GetType() const;

 protected:
  const DataType type_;
};

}  // namespace brave_ads::ml

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_DATA_DATA_H_
