/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_DATA_DATA_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_DATA_DATA_H_

#include "bat/ads/internal/ml/data/data_types.h"

namespace ads::ml {

class Data {
 public:
  explicit Data(const DataType& type);

  Data(const Data& other) = delete;
  Data& operator=(const Data& other) = delete;

  Data(Data&& other) noexcept = delete;
  Data& operator=(Data&& other) noexcept = delete;

  virtual ~Data();

  DataType GetType() const;

 protected:
  const DataType type_;
};

}  // namespace ads::ml

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_DATA_DATA_H_
