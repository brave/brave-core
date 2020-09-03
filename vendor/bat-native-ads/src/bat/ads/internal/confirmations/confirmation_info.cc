/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/confirmations/confirmation_info.h"

#include "bat/ads/internal/logging.h"

namespace ads {

ConfirmationInfo::ConfirmationInfo()
    : payment_token(nullptr),
      blinded_payment_token(nullptr) {}

ConfirmationInfo::ConfirmationInfo(
    const ConfirmationInfo& info) = default;

ConfirmationInfo::~ConfirmationInfo() = default;

bool ConfirmationInfo::operator==(
    const ConfirmationInfo& rhs) const {
  const std::string payment_token_base64 =
      payment_token.encode_base64();
  if (challenge_bypass_ristretto::exception_occurred()) {
    challenge_bypass_ristretto::TokenException e =
        challenge_bypass_ristretto::get_last_exception();
    BLOG(0, "Challenge Bypass Ristretto Error: " << e.what());
    return false;
  }

  const std::string rhs_payment_token_base64 =
      rhs.payment_token.encode_base64();
  if (challenge_bypass_ristretto::exception_occurred()) {
    challenge_bypass_ristretto::TokenException e =
        challenge_bypass_ristretto::get_last_exception();
    BLOG(0, "Challenge Bypass Ristretto Error: " << e.what());
    return false;
  }

  const std::string blinded_payment_token_base64 =
      blinded_payment_token.encode_base64();
  if (challenge_bypass_ristretto::exception_occurred()) {
    challenge_bypass_ristretto::TokenException e =
        challenge_bypass_ristretto::get_last_exception();
    BLOG(0, "Challenge Bypass Ristretto Error: " << e.what());
    return false;
  }

  const std::string rhs_blinded_payment_token_base64 =
      rhs.blinded_payment_token.encode_base64();
  if (challenge_bypass_ristretto::exception_occurred()) {
    challenge_bypass_ristretto::TokenException e =
        challenge_bypass_ristretto::get_last_exception();
    BLOG(0, "Challenge Bypass Ristretto Error: " << e.what());
    return false;
  }

  return id == rhs.id &&
      creative_instance_id == rhs.creative_instance_id &&
      type == rhs.type &&
      unblinded_token == rhs.unblinded_token &&
      payment_token_base64 == rhs_payment_token_base64 &&
      blinded_payment_token_base64 == rhs_blinded_payment_token_base64 &&
      credential == rhs.credential &&
      timestamp_in_seconds == rhs.timestamp_in_seconds &&
      created == rhs.created;
}

bool ConfirmationInfo::operator!=(
    const ConfirmationInfo& rhs) const {
  return !(*this == rhs);
}

bool ConfirmationInfo::IsValid() const {
  if (id.empty() ||
      type == ConfirmationType::kNone ||
      creative_instance_id.empty()) {
    return false;
  }

  return true;
}

}  // namespace ads
