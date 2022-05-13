/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/confirmations/confirmations_unittest_util.h"

#include "base/guid.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/account/confirmations/confirmation_info.h"
#include "bat/ads/internal/account/confirmations/confirmations_state.h"
#include "bat/ads/internal/account/redeem_unblinded_token/create_confirmation_util.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens_unittest_util.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "wrapper.hpp"

namespace ads {

using challenge_bypass_ristretto::BlindedToken;
using challenge_bypass_ristretto::PublicKey;
using challenge_bypass_ristretto::Token;
using challenge_bypass_ristretto::UnblindedToken;

ConfirmationInfo BuildConfirmation(const std::string& id,
                                   const std::string& transaction_id,
                                   const std::string& creative_instance_id,
                                   const ConfirmationType& type,
                                   const AdType& ad_type) {
  ConfirmationInfo confirmation;

  confirmation.id = id;
  confirmation.transaction_id = transaction_id;
  confirmation.creative_instance_id = creative_instance_id;
  confirmation.type = type;
  confirmation.ad_type = ad_type;

  privacy::UnblindedTokens* unblinded_tokens = privacy::get_unblinded_tokens();
  if (unblinded_tokens && !unblinded_tokens->IsEmpty()) {
    const privacy::UnblindedTokenInfo& unblinded_token =
        unblinded_tokens->GetToken();
    confirmation.unblinded_token = unblinded_token;
    unblinded_tokens->RemoveToken(unblinded_token);

    confirmation.tokens = {Token::decode_base64(
        R"(aXZNwft34oG2JAVBnpYh/ktTOzr2gi0lKosYNczUUz6ZS9gaDTJmU2FHFps9dIq+QoDwjSjctR5v0rRn+dYo+AHScVqFAgJ5t2s4KtSyawW10gk6hfWPQw16Q0+8u5AG)")};

    confirmation.blinded_tokens = {BlindedToken::decode_base64(
        "Ev5JE4/9TZI/5TqyN9JWfJ1To0HBwQw2rWeAPcdjX3Q=")};

    const std::string payload = CreateConfirmationRequestDTO(confirmation);
    confirmation.credential = CreateCredential(unblinded_token, payload);
  }

  confirmation.user_data = "";

  confirmation.created_at = Now();

  confirmation.was_created = false;

  return confirmation;
}

}  // namespace ads
