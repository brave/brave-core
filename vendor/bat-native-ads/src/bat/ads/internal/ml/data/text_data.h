/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_DATA_TEXT_DATA_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_DATA_TEXT_DATA_H_

#include <string>

#include "bat/ads/internal/ml/data/data.h"

namespace ads::ml {

class TextData final : public Data {
 public:
  TextData();
  explicit TextData(std::string text);

  const std::string& GetText() const;

 private:
  std::string text_;
};

}  // namespace ads::ml

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_DATA_TEXT_DATA_H_
