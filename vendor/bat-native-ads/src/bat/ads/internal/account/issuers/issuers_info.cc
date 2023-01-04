/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/issuers/issuers_info.h"

namespace ads {

IssuersInfo::IssuersInfo() = default;

IssuersInfo::IssuersInfo(const IssuersInfo& other) = default;

IssuersInfo& IssuersInfo::operator=(const IssuersInfo& other) = default;

IssuersInfo::IssuersInfo(IssuersInfo&& other) noexcept = default;

IssuersInfo& IssuersInfo::operator=(IssuersInfo&& other) noexcept = default;

IssuersInfo::~IssuersInfo() = default;

bool IssuersInfo::operator==(const IssuersInfo& other) const {
  return ping == other.ping && issuers == other.issuers;
}

bool IssuersInfo::operator!=(const IssuersInfo& other) const {
  return !(*this == other);
}

}  // namespace ads
