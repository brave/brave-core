/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"

#include "brave/components/brave_ads/core/internal/common/crypto/crypto_util.h"

namespace brave_ads {

WalletInfo::~WalletInfo() {
  crypto::SecureZero(payment_id);
  crypto::SecureZero(public_key_base64);
  crypto::SecureZero(secret_key_base64);
}

bool WalletInfo::IsValid() const {
  return !payment_id.empty() && !public_key_base64.empty() &&
         !secret_key_base64.empty();
}

}  // namespace brave_ads
