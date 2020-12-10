/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_ALIASES_H_  // NOLINT
#define BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_ALIASES_H_  // NOLINT

#include <deque>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace ads {

using TextClassificationProbabilitiesMap = std::map<std::string, double>;
using TextClassificationProbabilitiesList =
    std::deque<TextClassificationProbabilitiesMap>;

using SegmentProbabilityPair = std::pair<std::string, double>;
using SegmentProbabilitiesList = std::vector<SegmentProbabilityPair>;
using SegmentProbabilitiesMap = std::map<std::string, double>;

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_ALIASES_H_  // NOLINT
