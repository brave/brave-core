/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/data/text_data.h"

#include <utility>

namespace brave_ads::ml {

TextData::TextData() : Data(DataType::kText) {}

TextData::TextData(std::string text)
    : Data(DataType::kText), text_(std::move(text)) {}

const std::string& TextData::GetText() const {
  return text_;
}

}  // namespace brave_ads::ml
