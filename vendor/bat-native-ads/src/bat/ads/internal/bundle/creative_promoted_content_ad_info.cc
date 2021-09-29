/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/bundle/creative_promoted_content_ad_info.h"

namespace ads {

CreativePromotedContentAdInfo::CreativePromotedContentAdInfo() = default;

CreativePromotedContentAdInfo::CreativePromotedContentAdInfo(
    const CreativePromotedContentAdInfo& info) = default;

CreativePromotedContentAdInfo::~CreativePromotedContentAdInfo() = default;

bool CreativePromotedContentAdInfo::operator==(
    const CreativePromotedContentAdInfo& rhs) const {
  return title == rhs.title && description == rhs.description;
}

bool CreativePromotedContentAdInfo::operator!=(
    const CreativePromotedContentAdInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
