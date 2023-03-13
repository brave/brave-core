/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_TARGETING_MODELS_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_ALIAS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_TARGETING_MODELS_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_ALIAS_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/circular_deque.h"

namespace brave_ads::targeting {

using TextClassificationProbabilityMap = std::map<std::string, double>;
using TextClassificationProbabilityList =
    base::circular_deque<TextClassificationProbabilityMap>;

using SegmentProbabilityPair = std::pair<std::string, double>;
using SegmentProbabilityList = std::vector<SegmentProbabilityPair>;
using SegmentProbabilityMap = std::map<std::string, double>;

}  // namespace brave_ads::targeting

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_TARGETING_MODELS_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_ALIAS_H_
