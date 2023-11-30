/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ad_units/inline_content_ad/inline_content_ad_info.h"

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

bool InlineContentAdInfo::IsValid() const {
  return AdInfo::IsValid() && !title.empty() && !description.empty() &&
         image_url.is_valid() && !dimensions.empty() && !cta_text.empty();
}

}  // namespace brave_ads
