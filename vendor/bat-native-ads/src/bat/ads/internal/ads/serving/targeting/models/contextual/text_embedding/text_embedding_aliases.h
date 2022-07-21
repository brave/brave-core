/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_TARGETING_MODELS_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_ALIASES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_TARGETING_MODELS_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_ALIASES_H_

#include <vector>

#include "base/containers/circular_deque.h"
#include "bat/ads/internal/ml/data/vector_data.h"

namespace ads {
namespace targeting {

using TextEmbeddingList = base::circular_deque<ml::VectorData>;

}  // namespace targeting
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_TARGETING_MODELS_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_ALIASES_H_
