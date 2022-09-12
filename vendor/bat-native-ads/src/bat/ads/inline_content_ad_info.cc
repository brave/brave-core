/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/inline_content_ad_info.h"

namespace ads {

InlineContentAdInfo::InlineContentAdInfo() = default;

InlineContentAdInfo::InlineContentAdInfo(const InlineContentAdInfo& other) =
    default;

InlineContentAdInfo& InlineContentAdInfo::operator=(
    const InlineContentAdInfo& other) = default;

InlineContentAdInfo::InlineContentAdInfo(InlineContentAdInfo&& other) noexcept =
    default;

InlineContentAdInfo& InlineContentAdInfo::operator=(
    InlineContentAdInfo&& other) noexcept = default;

InlineContentAdInfo::~InlineContentAdInfo() = default;

bool InlineContentAdInfo::operator==(const InlineContentAdInfo& rhs) const {
  return AdInfo::operator==(rhs) && title == rhs.title &&
         description == rhs.description && image_url == rhs.image_url &&
         dimensions == rhs.dimensions && cta_text == rhs.cta_text;
}

bool InlineContentAdInfo::operator!=(const InlineContentAdInfo& rhs) const {
  return !(*this == rhs);
}

bool InlineContentAdInfo::IsValid() const {
  if (!AdInfo::IsValid()) {
    return false;
  }

  if (title.empty() || description.empty() || !image_url.is_valid() ||
      dimensions.empty() || cta_text.empty()) {
    return false;
  }

  return true;
}

}  // namespace ads
