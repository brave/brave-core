/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/bundle/creative_inline_content_ad_info.h"

namespace ads {

CreativeInlineContentAdInfo::CreativeInlineContentAdInfo() = default;

CreativeInlineContentAdInfo::CreativeInlineContentAdInfo(
    const CreativeInlineContentAdInfo& info) = default;

CreativeInlineContentAdInfo::~CreativeInlineContentAdInfo() = default;

bool CreativeInlineContentAdInfo::operator==(
    const CreativeInlineContentAdInfo& rhs) const {
  return title == rhs.title && description == rhs.description &&
         image_url == rhs.image_url && dimensions == rhs.dimensions &&
         cta_text == rhs.cta_text;
}

bool CreativeInlineContentAdInfo::operator!=(
    const CreativeInlineContentAdInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
