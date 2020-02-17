/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/token_info.h"

namespace confirmations {

TokenInfo::TokenInfo() :
    unblinded_token(nullptr) {}

TokenInfo::TokenInfo(
    const TokenInfo& info) = default;

TokenInfo::~TokenInfo() = default;

}  // namespace confirmations
