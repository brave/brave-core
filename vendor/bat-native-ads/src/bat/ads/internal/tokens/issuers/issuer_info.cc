/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/issuers/issuer_info.h"

namespace ads {

IssuerInfo::IssuerInfo() = default;

IssuerInfo::IssuerInfo(const IssuerInfo& info) = default;

IssuerInfo::~IssuerInfo() = default;

bool IssuerInfo::operator==(const IssuerInfo& rhs) const {
  return type == rhs.type && public_keys == rhs.public_keys;
}

bool IssuerInfo::operator!=(const IssuerInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
