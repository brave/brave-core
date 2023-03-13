/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_DATA_TEXT_DATA_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_DATA_TEXT_DATA_H_

#include <string>

#include "brave/components/brave_ads/core/internal/ml/data/data.h"

namespace brave_ads::ml {

class TextData final : public Data {
 public:
  TextData();
  explicit TextData(std::string text);

  const std::string& GetText() const;

 private:
  std::string text_;
};

}  // namespace brave_ads::ml

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_DATA_TEXT_DATA_H_
