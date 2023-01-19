/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_INFO_H_

#include <string>

#include "bat/ads/ad_info.h"

namespace ads {

struct SearchResultAdInfo final : AdInfo {
  bool IsValid() const;

  std::string headline_text;
  std::string description;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_INFO_H_
