/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_CLASSIFICATION_MODEL_TEXT_CLASSIFICATION_ALIAS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_CLASSIFICATION_MODEL_TEXT_CLASSIFICATION_ALIAS_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/circular_deque.h"

namespace brave_ads {

using TextClassificationProbabilityMap =
    std::map</*segment*/ std::string, /*page_score*/ double>;
using TextClassificationProbabilityList =
    base::circular_deque<TextClassificationProbabilityMap>;

using SegmentProbabilityPair =
    std::pair</*segment*/ std::string, /*probability*/ double>;
using SegmentProbabilityList = std::vector<SegmentProbabilityPair>;
using SegmentProbabilityMap =
    std::map</*segment*/ std::string, /*page_score*/ double>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_CLASSIFICATION_MODEL_TEXT_CLASSIFICATION_ALIAS_H_
