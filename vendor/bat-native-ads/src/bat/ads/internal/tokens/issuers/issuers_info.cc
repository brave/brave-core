/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/issuers/issuers_info.h"

namespace ads {

IssuersInfo::IssuersInfo() = default;

IssuersInfo::IssuersInfo(const IssuersInfo& info) = default;

IssuersInfo::~IssuersInfo() = default;

bool IssuersInfo::operator==(const IssuersInfo& rhs) const {
  return ping == rhs.ping && issuers == rhs.issuers;
}

bool IssuersInfo::operator!=(const IssuersInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
