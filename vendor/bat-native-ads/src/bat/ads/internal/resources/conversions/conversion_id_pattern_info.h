/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_CONVERSIONS_CONVERSION_ID_PATTERN_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_CONVERSIONS_CONVERSION_ID_PATTERN_INFO_H_

#include <string>

namespace ads {

struct ConversionIdPatternInfo final {
  ConversionIdPatternInfo();
  ConversionIdPatternInfo(const ConversionIdPatternInfo& info);
  ~ConversionIdPatternInfo();

  bool operator==(const ConversionIdPatternInfo& rhs) const;
  bool operator!=(const ConversionIdPatternInfo& rhs) const;

  std::string id_pattern;
  std::string url_pattern;
  std::string search_in;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_CONVERSIONS_CONVERSION_ID_PATTERN_INFO_H_
