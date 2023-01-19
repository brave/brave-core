/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"

namespace ads {

CreativeInlineContentAdInfo::CreativeInlineContentAdInfo() = default;

CreativeInlineContentAdInfo::CreativeInlineContentAdInfo(
    const CreativeAdInfo& creative_ad)
    : CreativeAdInfo(creative_ad) {}

CreativeInlineContentAdInfo::CreativeInlineContentAdInfo(
    const CreativeInlineContentAdInfo& other) = default;

CreativeInlineContentAdInfo& CreativeInlineContentAdInfo::operator=(
    const CreativeInlineContentAdInfo& other) = default;

CreativeInlineContentAdInfo::CreativeInlineContentAdInfo(
    CreativeInlineContentAdInfo&& other) noexcept = default;

CreativeInlineContentAdInfo& CreativeInlineContentAdInfo::operator=(
    CreativeInlineContentAdInfo&& other) noexcept = default;

CreativeInlineContentAdInfo::~CreativeInlineContentAdInfo() = default;

bool CreativeInlineContentAdInfo::operator==(
    const CreativeInlineContentAdInfo& other) const {
  return CreativeAdInfo::operator==(other) && title == other.title &&
         description == other.description && image_url == other.image_url &&
         dimensions == other.dimensions && cta_text == other.cta_text;
}

bool CreativeInlineContentAdInfo::operator!=(
    const CreativeInlineContentAdInfo& other) const {
  return !(*this == other);
}

}  // namespace ads
