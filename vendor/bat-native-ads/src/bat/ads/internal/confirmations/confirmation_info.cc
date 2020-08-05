/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/confirmations/confirmation_info.h"

namespace ads {

ConfirmationInfo::ConfirmationInfo()
    : payment_token(nullptr),
      blinded_payment_token(nullptr) {}

ConfirmationInfo::ConfirmationInfo(
    const ConfirmationInfo& info) = default;

ConfirmationInfo::~ConfirmationInfo() = default;

bool ConfirmationInfo::operator==(
    const ConfirmationInfo& rhs) const {
  return id == rhs.id &&
      creative_instance_id == rhs.creative_instance_id &&
      type == rhs.type &&
      unblinded_token == rhs.unblinded_token &&
      payment_token.encode_base64() == rhs.payment_token.encode_base64() &&
      blinded_payment_token.encode_base64() ==
          rhs.blinded_payment_token.encode_base64() &&
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
