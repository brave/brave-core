/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/confirmation_info.h"

namespace confirmations {

ConfirmationInfo::ConfirmationInfo()
    : type(ConfirmationType::kNone),
      payment_token(nullptr),
      blinded_payment_token(nullptr),
      timestamp_in_seconds(0),
      created(false) {}

ConfirmationInfo::ConfirmationInfo(
    const ConfirmationInfo& info) = default;

ConfirmationInfo::~ConfirmationInfo() = default;

bool ConfirmationInfo::operator==(
    const ConfirmationInfo info) const {
  return id == info.id &&
      creative_instance_id == info.creative_instance_id &&
      type == info.type &&
      token_info == info.token_info &&
      payment_token.encode_base64() == info.payment_token.encode_base64() &&
      blinded_payment_token.encode_base64() ==
          info.blinded_payment_token.encode_base64() &&
      credential == info.credential &&
      timestamp_in_seconds == info.timestamp_in_seconds &&
      created == info.created;
}

bool ConfirmationInfo::operator!=(
    const ConfirmationInfo info) const {
  return !(*this == info);
}

}  // namespace confirmations
