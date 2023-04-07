// /* Copyright (c) 2022 The Brave Authors. All rights reserved.
//  * This Source Code Form is subject to the terms of the Mozilla Public
//  * License, v. 2.0. If a copy of the MPL was not distributed with this file,
//  * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/creatives_info.h"

namespace brave_ads {

CreativesInfo::CreativesInfo() = default;

CreativesInfo::CreativesInfo(const CreativesInfo& other) = default;

CreativesInfo& CreativesInfo::operator=(const CreativesInfo& other) = default;

CreativesInfo::CreativesInfo(CreativesInfo&& other) noexcept = default;

CreativesInfo& CreativesInfo::operator=(CreativesInfo&& other) noexcept =
    default;

CreativesInfo::~CreativesInfo() = default;

}  // namespace brave_ads
