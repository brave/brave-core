/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ISSUERS_ISSUER_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ISSUERS_ISSUER_INFO_H_

#include "bat/ads/internal/account/issuers/issuer_types.h"
#include "bat/ads/internal/account/issuers/public_key_aliases.h"

namespace ads {

struct IssuerInfo {
  IssuerInfo();
  IssuerInfo(const IssuerInfo& info);
  ~IssuerInfo();

  bool operator==(const IssuerInfo& rhs) const;
  bool operator!=(const IssuerInfo& rhs) const;

  IssuerType type = IssuerType::kUndefined;
  PublicKeyMap public_keys;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ISSUERS_ISSUER_INFO_H_
