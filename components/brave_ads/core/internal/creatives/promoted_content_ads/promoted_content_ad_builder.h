/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_BUILDER_H_

#include <string>

namespace brave_ads {

struct CreativePromotedContentAdInfo;
struct PromotedContentAdInfo;

PromotedContentAdInfo BuildPromotedContentAd(
    const CreativePromotedContentAdInfo& creative_promoted_content_ad);

PromotedContentAdInfo BuildPromotedContentAd(
    const CreativePromotedContentAdInfo& creative_promoted_content_ad,
    const std::string& placement_id);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_BUILDER_H_
