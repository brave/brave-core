/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/transformation/lowercase_transformation.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/ml/data/data.h"
#include "brave/components/brave_ads/core/internal/ml/data/text_data.h"

namespace brave_ads::ml {

LowercaseTransformation::LowercaseTransformation()
    : Transformation(TransformationType::kLowercase) {}

std::unique_ptr<Data> LowercaseTransformation::Apply(
    const std::unique_ptr<Data>& input_data) const {
  CHECK(input_data->GetType() == DataType::kText);

  auto* text_data = static_cast<TextData*>(input_data.get());

  std::string lowercase_text = base::ToLowerASCII(text_data->GetText());

  return std::make_unique<TextData>(std::move(lowercase_text));
}

}  // namespace brave_ads::ml
