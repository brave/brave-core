/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/redeem_token_mock.h"

#include "bat/confirmations/internal/confirmations_impl.h"
#include "bat/confirmations/internal/unblinded_tokens.h"

namespace confirmations {

RedeemTokenMock::RedeemTokenMock(
    ConfirmationsImpl* confirmations,
    UnblindedTokens* unblinded_tokens,
    UnblindedTokens* unblinded_payment_tokens)
    : RedeemToken(confirmations, unblinded_tokens, unblinded_payment_tokens) {
}

RedeemTokenMock::~RedeemTokenMock() = default;

}  // namespace confirmations
