/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/bundle/creative_search_result_ad_info.h"

namespace ads {

CreativeSearchResultAdInfo::CreativeSearchResultAdInfo() = default;

CreativeSearchResultAdInfo::CreativeSearchResultAdInfo(
    const CreativeSearchResultAdInfo& info) = default;

CreativeSearchResultAdInfo::CreativeSearchResultAdInfo(
    const CreativeAdInfo& creative_ad)
    : CreativeAdInfo(creative_ad) {}

CreativeSearchResultAdInfo::~CreativeSearchResultAdInfo() = default;

bool CreativeSearchResultAdInfo::operator==(
    const CreativeSearchResultAdInfo& rhs) const {
  return title == rhs.title && body == rhs.body;
}

bool CreativeSearchResultAdInfo::operator!=(
    const CreativeSearchResultAdInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
