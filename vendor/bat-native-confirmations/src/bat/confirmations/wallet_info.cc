/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/wallet_info.h"

namespace confirmations {

WalletInfo::WalletInfo() = default;

WalletInfo::WalletInfo(
    const WalletInfo& info) = default;

WalletInfo::~WalletInfo() = default;

bool WalletInfo::IsValid() const {
  return !payment_id.empty() && !private_key.empty();
}

bool WalletInfo::operator==(
    const WalletInfo& info) const {
  return (payment_id == info.payment_id && private_key == info.private_key);
}

}  // namespace confirmations
