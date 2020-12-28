/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_DATA_DATA_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_DATA_DATA_H_

namespace ads {
namespace ml {

enum class DataType { TEXT_DATA = 0, VECTOR_DATA = 1 };

class Data {
 public:
  explicit Data(const DataType& type);

  virtual ~Data();

  DataType GetType() const;

 protected:
  const DataType type_;
};

}  // namespace ml
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_DATA_DATA_H_
