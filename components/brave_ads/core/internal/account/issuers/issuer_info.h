/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUER_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUER_INFO_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuer_types.h"

namespace brave_ads {

using IssuerPublicKeyMap =
    base::flat_map</*public_key*/ std::string, /*associated_value*/ double>;

struct IssuerInfo final {
  IssuerInfo();

  IssuerInfo(const IssuerInfo&);
  IssuerInfo& operator=(const IssuerInfo&);

  IssuerInfo(IssuerInfo&&) noexcept;
  IssuerInfo& operator=(IssuerInfo&&) noexcept;

  ~IssuerInfo();

  bool operator==(const IssuerInfo&) const = default;

  IssuerType type = IssuerType::kUndefined;
  IssuerPublicKeyMap public_keys;
};

using IssuerList = std::vector<IssuerInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUER_INFO_H_
