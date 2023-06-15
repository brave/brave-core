/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/opted_in_user_data_info.h"

#include <tuple>

namespace brave_ads {

OptedInUserDataInfo::OptedInUserDataInfo() = default;

OptedInUserDataInfo::OptedInUserDataInfo(const OptedInUserDataInfo& other) {
  *this = other;
}

OptedInUserDataInfo& OptedInUserDataInfo::operator=(
    const OptedInUserDataInfo& other) {
  if (this != &other) {
    dynamic = other.dynamic.Clone();
    fixed = other.fixed.Clone();
  }

  return *this;
}

OptedInUserDataInfo::OptedInUserDataInfo(OptedInUserDataInfo&& other) noexcept =
    default;

OptedInUserDataInfo& OptedInUserDataInfo::operator=(
    OptedInUserDataInfo&& other) noexcept = default;

OptedInUserDataInfo::~OptedInUserDataInfo() = default;

bool operator==(const OptedInUserDataInfo& lhs,
                const OptedInUserDataInfo& rhs) {
  const auto tie = [](const OptedInUserDataInfo& opted_in_user_data) {
    return std::tie(opted_in_user_data.dynamic, opted_in_user_data.fixed);
  };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const OptedInUserDataInfo& lhs,
                const OptedInUserDataInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
