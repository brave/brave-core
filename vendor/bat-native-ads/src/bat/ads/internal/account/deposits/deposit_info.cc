/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/deposits/deposit_info.h"

namespace ads {

DepositInfo::DepositInfo() = default;

DepositInfo::~DepositInfo() = default;

DepositInfo::DepositInfo(DepositInfo&&) noexcept = default;

DepositInfo& DepositInfo::operator=(DepositInfo&&) noexcept = default;

bool DepositInfo::IsValid() const {
  return !creative_instance_id.empty();
}

}  // namespace ads
