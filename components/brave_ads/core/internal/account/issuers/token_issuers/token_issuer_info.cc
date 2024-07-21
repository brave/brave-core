/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_info.h"

namespace brave_ads {

TokenIssuerInfo::TokenIssuerInfo() = default;

TokenIssuerInfo::TokenIssuerInfo(const TokenIssuerInfo& other) = default;

TokenIssuerInfo& TokenIssuerInfo::operator=(const TokenIssuerInfo& other) =
    default;

TokenIssuerInfo::TokenIssuerInfo(TokenIssuerInfo&& other) noexcept = default;

TokenIssuerInfo& TokenIssuerInfo::operator=(TokenIssuerInfo&& other) noexcept =
    default;

TokenIssuerInfo::~TokenIssuerInfo() = default;

}  // namespace brave_ads
