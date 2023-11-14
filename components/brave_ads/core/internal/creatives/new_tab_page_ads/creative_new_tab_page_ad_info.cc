/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"

namespace brave_ads {

CreativeNewTabPageAdInfo::CreativeNewTabPageAdInfo() = default;

CreativeNewTabPageAdInfo::CreativeNewTabPageAdInfo(
    const CreativeAdInfo& creative_ad)
    : CreativeAdInfo(creative_ad) {}

CreativeNewTabPageAdInfo::CreativeNewTabPageAdInfo(
    const CreativeNewTabPageAdInfo& other) = default;

CreativeNewTabPageAdInfo& CreativeNewTabPageAdInfo::operator=(
    const CreativeNewTabPageAdInfo& other) = default;

CreativeNewTabPageAdInfo::CreativeNewTabPageAdInfo(
    CreativeNewTabPageAdInfo&& other) noexcept = default;

CreativeNewTabPageAdInfo& CreativeNewTabPageAdInfo::operator=(
    CreativeNewTabPageAdInfo&& other) noexcept = default;

CreativeNewTabPageAdInfo::~CreativeNewTabPageAdInfo() = default;

}  // namespace brave_ads
