/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/user_data_info.h"

namespace brave_ads {

UserDataInfo::UserDataInfo() = default;

UserDataInfo::UserDataInfo(const UserDataInfo& other) {
  *this = other;
}

UserDataInfo& UserDataInfo::operator=(const UserDataInfo& other) {
  if (this != &other) {
    dynamic = other.dynamic.Clone();
    fixed = other.fixed.Clone();
  }

  return *this;
}

UserDataInfo::UserDataInfo(UserDataInfo&& other) noexcept = default;

UserDataInfo& UserDataInfo::operator=(UserDataInfo&& other) noexcept = default;

UserDataInfo::~UserDataInfo() = default;

}  // namespace brave_ads
