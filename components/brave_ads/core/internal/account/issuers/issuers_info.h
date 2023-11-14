/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_INFO_H_

#include "brave/components/brave_ads/core/internal/account/issuers/issuer_info.h"

namespace brave_ads {

struct IssuersInfo final {
  IssuersInfo();

  IssuersInfo(const IssuersInfo&);
  IssuersInfo& operator=(const IssuersInfo&);

  IssuersInfo(IssuersInfo&&) noexcept;
  IssuersInfo& operator=(IssuersInfo&&) noexcept;

  ~IssuersInfo();

  bool operator==(const IssuersInfo&) const = default;

  int ping = 0;
  IssuerList issuers;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_INFO_H_
