/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"

namespace brave_ads {

PaymentTokenInfo::PaymentTokenInfo() = default;

PaymentTokenInfo::PaymentTokenInfo(const PaymentTokenInfo& other) = default;

PaymentTokenInfo& PaymentTokenInfo::operator=(const PaymentTokenInfo& other) =
    default;

PaymentTokenInfo::PaymentTokenInfo(PaymentTokenInfo&& other) noexcept = default;

PaymentTokenInfo& PaymentTokenInfo::operator=(
    PaymentTokenInfo&& other) noexcept = default;

PaymentTokenInfo::~PaymentTokenInfo() = default;

}  // namespace brave_ads
