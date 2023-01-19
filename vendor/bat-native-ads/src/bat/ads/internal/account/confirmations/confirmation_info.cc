/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/confirmations/confirmation_info.h"

#include "bat/ads/internal/common/numbers/number_util.h"

namespace ads {

ConfirmationInfo::ConfirmationInfo() = default;

ConfirmationInfo::ConfirmationInfo(const ConfirmationInfo& other) = default;

ConfirmationInfo& ConfirmationInfo::operator=(const ConfirmationInfo& other) =
    default;

ConfirmationInfo::ConfirmationInfo(ConfirmationInfo&& other) noexcept = default;

ConfirmationInfo& ConfirmationInfo::operator=(
    ConfirmationInfo&& other) noexcept = default;

ConfirmationInfo::~ConfirmationInfo() = default;

bool operator==(const ConfirmationInfo& lhs, const ConfirmationInfo& rhs) {
  return lhs.transaction_id == rhs.transaction_id &&
         lhs.creative_instance_id == rhs.creative_instance_id &&
         lhs.type == rhs.type && lhs.ad_type == rhs.ad_type &&
         DoubleEquals(lhs.created_at.ToDoubleT(), rhs.created_at.ToDoubleT()) &&
         lhs.was_created == rhs.was_created && lhs.opted_in == rhs.opted_in;
}

bool operator!=(const ConfirmationInfo& lhs, const ConfirmationInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace ads
