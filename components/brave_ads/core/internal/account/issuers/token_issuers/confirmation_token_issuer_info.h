/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_TOKEN_ISSUERS_CONFIRMATION_TOKEN_ISSUER_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_TOKEN_ISSUERS_CONFIRMATION_TOKEN_ISSUER_INFO_H_

#include <string>

#include "base/containers/flat_set.h"

namespace brave_ads {

struct ConfirmationTokenIssuerInfo final {
  ConfirmationTokenIssuerInfo();

  ConfirmationTokenIssuerInfo(const ConfirmationTokenIssuerInfo&);
  ConfirmationTokenIssuerInfo& operator=(const ConfirmationTokenIssuerInfo&);

  ConfirmationTokenIssuerInfo(ConfirmationTokenIssuerInfo&&) noexcept;
  ConfirmationTokenIssuerInfo& operator=(
      ConfirmationTokenIssuerInfo&&) noexcept;

  ~ConfirmationTokenIssuerInfo();

  bool operator==(const ConfirmationTokenIssuerInfo&) const = default;

  base::flat_set<std::string> public_keys;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_TOKEN_ISSUERS_CONFIRMATION_TOKEN_ISSUER_INFO_H_
