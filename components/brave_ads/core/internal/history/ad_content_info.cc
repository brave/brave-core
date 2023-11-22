/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/history/ad_content_info.h"

namespace brave_ads {

AdContentInfo::AdContentInfo() = default;

AdContentInfo::AdContentInfo(const AdContentInfo& other) = default;

AdContentInfo& AdContentInfo::operator=(const AdContentInfo& other) = default;

AdContentInfo::AdContentInfo(AdContentInfo&& other) noexcept = default;

AdContentInfo& AdContentInfo::operator=(AdContentInfo&& other) noexcept =
    default;

AdContentInfo::~AdContentInfo() = default;

}  // namespace brave_ads
