/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/data/text_data.h"

namespace ads {
namespace ml {

TextData::TextData() : Data(DataType::kText) {}

TextData::TextData(const TextData& text_data) : Data(DataType::kText) {
  text_ = text_data.GetText();
}

TextData::TextData(const std::string& text)
    : Data(DataType::kText), text_(text) {}

std::string TextData::GetText() const {
  return text_;
}

TextData::~TextData() = default;

TextData& TextData::operator=(const TextData& text_data) {
  text_ = text_data.GetText();
  return *this;
}

}  // namespace ml
}  // namespace ads
