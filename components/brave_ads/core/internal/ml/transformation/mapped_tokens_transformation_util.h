/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_TRANSFORMATION_MAPPED_TOKENS_TRANSFORMATION_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_TRANSFORMATION_MAPPED_TOKENS_TRANSFORMATION_UTIL_H_

#include <memory>
#include <string>

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"

namespace brave_ads::ml {

absl::optional<std::basic_string<unsigned char>> CompressToken(
    const std::string& token,
    std::map<std::string, std::vector<int>> huffman_coding_mapping);

}  // namespace brave_ads::ml

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_TRANSFORMATION_MAPPED_TOKENS_TRANSFORMATION_UTIL_H_
