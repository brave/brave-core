/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/issuers/public_key_util.h"

#include "bat/ads/internal/account/issuers/issuer_info.h"

namespace ads {

bool PublicKeyExists(const IssuerInfo& issuer, const std::string& public_key) {
  return issuer.public_keys.find(public_key) != issuer.public_keys.cend();
}

}  // namespace ads
