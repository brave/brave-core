/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/security/key_pair_info.h"

namespace ads {
namespace security {

KeyPairInfo::KeyPairInfo() = default;

KeyPairInfo::KeyPairInfo(const KeyPairInfo& info) = default;

KeyPairInfo::~KeyPairInfo() = default;

bool KeyPairInfo::operator==(const KeyPairInfo& rhs) const {
  return public_key == rhs.public_key && secret_key == rhs.secret_key;
}

bool KeyPairInfo::operator!=(const KeyPairInfo& rhs) const {
  return !(*this == rhs);
}

bool KeyPairInfo::IsValid() const {
  if (public_key.empty() || secret_key.empty()) {
    return false;
  }

  return true;
}

}  // namespace security
}  // namespace ads
