/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUER_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUER_INFO_H_

#include <vector>

#include "brave/components/brave_ads/core/internal/account/issuers/issuer_types.h"
#include "brave/components/brave_ads/core/internal/account/issuers/public_key_alias.h"

namespace brave_ads {

struct IssuerInfo final {
  IssuerInfo();

  IssuerInfo(const IssuerInfo&);
  IssuerInfo& operator=(const IssuerInfo&);

  IssuerInfo(IssuerInfo&&) noexcept;
  IssuerInfo& operator=(IssuerInfo&&) noexcept;

  ~IssuerInfo();

  bool operator==(const IssuerInfo&) const = default;

  IssuerType type = IssuerType::kUndefined;
  PublicKeyMap public_keys;
};

using IssuerList = std::vector<IssuerInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUER_INFO_H_
