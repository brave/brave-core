/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/user_model_info.h"

namespace brave_ads {

UserModelInfo::UserModelInfo() = default;

UserModelInfo::UserModelInfo(const UserModelInfo& other) = default;

UserModelInfo& UserModelInfo::operator=(const UserModelInfo& other) = default;

UserModelInfo::UserModelInfo(UserModelInfo&& other) noexcept = default;

UserModelInfo& UserModelInfo::operator=(UserModelInfo&& other) noexcept =
    default;

UserModelInfo::~UserModelInfo() = default;

}  // namespace brave_ads
