/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_ALIASES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_ALIASES_H_

#include <functional>

#include "bat/ads/internal/bundle/creative_inline_content_ad_info_aliases.h"

namespace ads {
namespace inline_content_ads {

using GetEligibleAdsCallback =
    std::function<void(const bool, const CreativeInlineContentAdList&)>;

using GetEligibleAdsV2Callback =
    std::function<void(const bool,
                       const absl::optional<CreativeInlineContentAdInfo>&)>;

}  // namespace inline_content_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_ALIASES_H_
