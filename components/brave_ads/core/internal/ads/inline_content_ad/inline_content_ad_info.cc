/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ads/inline_content_ad_info.h"

#include <tuple>

namespace brave_ads {

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

bool InlineContentAdInfo::operator==(const InlineContentAdInfo& other) const {
  const auto tie = [](const InlineContentAdInfo& ad) {
    return std::tie(ad.title, ad.description, ad.image_url, ad.dimensions,
                    ad.cta_text);
  };

  return AdInfo::operator==(other) && tie(*this) == tie(other);
}

bool InlineContentAdInfo::operator!=(const InlineContentAdInfo& other) const {
  return !(*this == other);
}

bool InlineContentAdInfo::IsValid() const {
  return AdInfo::IsValid() && !title.empty() && !description.empty() &&
         image_url.is_valid() && !dimensions.empty() && !cta_text.empty();
}

}  // namespace brave_ads
