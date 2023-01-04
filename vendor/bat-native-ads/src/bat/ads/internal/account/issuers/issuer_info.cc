/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/issuers/issuer_info.h"

namespace ads {

IssuerInfo::IssuerInfo() = default;

IssuerInfo::IssuerInfo(const IssuerInfo& other) = default;

IssuerInfo& IssuerInfo::operator=(const IssuerInfo& other) = default;

IssuerInfo::IssuerInfo(IssuerInfo&& other) noexcept = default;

IssuerInfo& IssuerInfo::operator=(IssuerInfo&& other) noexcept = default;

IssuerInfo::~IssuerInfo() = default;

bool IssuerInfo::operator==(const IssuerInfo& other) const {
  return type == other.type && public_keys == other.public_keys;
}

bool IssuerInfo::operator!=(const IssuerInfo& other) const {
  return !(*this == other);
}

}  // namespace ads
