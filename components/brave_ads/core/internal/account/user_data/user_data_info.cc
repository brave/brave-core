/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/user_data_info.h"

#include <tuple>

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

bool operator==(const UserDataInfo& lhs, const UserDataInfo& rhs) {
  const auto tie = [](const UserDataInfo& user_data) {
    return std::tie(user_data.dynamic, user_data.fixed);
  };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const UserDataInfo& lhs, const UserDataInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
