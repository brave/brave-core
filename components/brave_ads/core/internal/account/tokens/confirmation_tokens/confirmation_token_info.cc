/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_token_info.h"

namespace brave_ads {

bool ConfirmationTokenInfo::IsValid() const {
  return unblinded_token.has_value() && public_key.has_value() &&
         !signature_base64.empty();
}

}  // namespace brave_ads
