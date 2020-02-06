/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_AD_CONVERSION_INFO_H_
#define BAT_ADS_AD_CONVERSION_INFO_H_

#include <stdint.h>
#include <string>

#include "bat/ads/export.h"

namespace ads {

struct ADS_EXPORT AdConversionInfo {
  uint64_t timestamp_in_seconds;
  std::string creative_set_id;
  std::string uuid;
};

}  // namespace ads

#endif  // BAT_ADS_AD_CONVERSION_INFO_H_
