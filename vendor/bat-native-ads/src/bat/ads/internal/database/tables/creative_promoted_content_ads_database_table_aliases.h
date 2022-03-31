/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CREATIVE_PROMOTED_CONTENT_ADS_DATABASE_TABLE_ALIASES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CREATIVE_PROMOTED_CONTENT_ADS_DATABASE_TABLE_ALIASES_H_

#include <functional>
#include <string>
#include <vector>

#include "bat/ads/internal/bundle/creative_promoted_content_ad_info_aliases.h"

namespace ads {

using GetCreativePromotedContentAdCallback =
    std::function<void(const bool,
                       const std::string& creative_instance_id,
                       const CreativePromotedContentAdInfo&)>;

using GetCreativePromotedContentAdsCallback =
    std::function<void(const bool,
                       const std::vector<std::string>&,
                       const CreativePromotedContentAdList&)>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CREATIVE_PROMOTED_CONTENT_ADS_DATABASE_TABLE_ALIASES_H_
