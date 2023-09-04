/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"

#include <tuple>

namespace brave_ads {

ConfirmationInfo::ConfirmationInfo() = default;

ConfirmationInfo::ConfirmationInfo(const ConfirmationInfo& other) = default;

ConfirmationInfo& ConfirmationInfo::operator=(const ConfirmationInfo& other) =
    default;

ConfirmationInfo::ConfirmationInfo(ConfirmationInfo&& other) noexcept = default;

ConfirmationInfo& ConfirmationInfo::operator=(
    ConfirmationInfo&& other) noexcept = default;

ConfirmationInfo::~ConfirmationInfo() = default;

bool operator==(const ConfirmationInfo& lhs, const ConfirmationInfo& rhs) {
  const auto tie = [](const ConfirmationInfo& confirmation) {
    return std::tie(
        confirmation.transaction_id, confirmation.creative_instance_id,
        confirmation.type, confirmation.ad_type, confirmation.created_at,
        confirmation.was_created, confirmation.reward, confirmation.user_data);
  };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const ConfirmationInfo& lhs, const ConfirmationInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
