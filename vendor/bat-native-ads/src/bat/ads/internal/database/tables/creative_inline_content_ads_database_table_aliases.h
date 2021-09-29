/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CREATIVE_INLINE_CONTENT_ADS_DATABASE_TABLE_ALIASES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CREATIVE_INLINE_CONTENT_ADS_DATABASE_TABLE_ALIASES_H_

#include <functional>
#include <string>

#include "bat/ads/internal/bundle/creative_inline_content_ad_info_aliases.h"
#include "bat/ads/internal/segments/segments_aliases.h"

namespace ads {

using GetCreativeInlineContentAdCallback =
    std::function<void(const bool success,
                       const std::string& creative_instance_id,
                       const CreativeInlineContentAdInfo& ad)>;

using GetCreativeInlineContentAdsCallback =
    std::function<void(const bool success,
                       const SegmentList& segments,
                       const CreativeInlineContentAdList& ads)>;

using GetCreativeInlineContentAdsForDimensionsCallback =
    std::function<void(const bool success,
                       const CreativeInlineContentAdList& ads)>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CREATIVE_INLINE_CONTENT_ADS_DATABASE_TABLE_ALIASES_H_
