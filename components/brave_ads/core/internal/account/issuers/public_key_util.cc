/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/public_key_util.h"

#include <optional>
#include <string>

#include "base/containers/contains.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuer_info.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"

namespace brave_ads {

bool PublicKeyExists(const IssuerInfo& issuer,
                     const cbr::PublicKey& public_key) {
  const std::optional<std::string> public_key_base64 =
      public_key.EncodeBase64();

  return public_key_base64 &&
         base::Contains(issuer.public_keys, public_key_base64.value());
}

}  // namespace brave_ads
