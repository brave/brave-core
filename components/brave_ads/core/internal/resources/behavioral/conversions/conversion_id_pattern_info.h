/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_CONVERSIONS_CONVERSION_ID_PATTERN_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_CONVERSIONS_CONVERSION_ID_PATTERN_INFO_H_

#include <map>
#include <string>

namespace brave_ads {

struct ConversionIdPatternInfo final {
  std::string id_pattern;
  std::string url_pattern;
  std::string search_in;
};

bool operator==(const ConversionIdPatternInfo&, const ConversionIdPatternInfo&);
bool operator!=(const ConversionIdPatternInfo&, const ConversionIdPatternInfo&);

using ConversionIdPatternMap = std::map<std::string, ConversionIdPatternInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_CONVERSIONS_CONVERSION_ID_PATTERN_INFO_H_
