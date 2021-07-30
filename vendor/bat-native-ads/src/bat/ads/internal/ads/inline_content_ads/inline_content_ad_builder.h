/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_BUILDER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_BUILDER_H_

#include <string>

namespace ads {

struct InlineContentAdInfo;
struct CreativeInlineContentAdInfo;

InlineContentAdInfo BuildInlineContentAd(
    const CreativeInlineContentAdInfo& creative_inline_content_ad);

InlineContentAdInfo BuildInlineContentAd(
    const CreativeInlineContentAdInfo& creative_inline_content_ad,
    const std::string& uuid);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_BUILDER_H_
