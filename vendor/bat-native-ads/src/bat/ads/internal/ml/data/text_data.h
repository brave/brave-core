/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_DATA_TEXT_DATA_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_DATA_TEXT_DATA_H_

#include <string>

#include "bat/ads/internal/ml/data/data.h"

namespace ads {
namespace ml {

class TextData : public Data {
 public:
  TextData();

  TextData(const TextData& text_data);

  // Explicit copy assignment operator is required because the class
  // inherits const member type_ that cannot be copied by default
  TextData& operator=(const TextData& text_data);

  explicit TextData(const std::string& text);

  ~TextData() override;

  std::string GetText() const;

 private:
  std::string text_;
};

}  // namespace ml
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_DATA_TEXT_DATA_H_
