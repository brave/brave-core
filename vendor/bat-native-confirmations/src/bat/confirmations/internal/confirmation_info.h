/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_CONFIRMATION_INFO_H_
#define BAT_CONFIRMATIONS_INTERNAL_CONFIRMATION_INFO_H_

#include <string>

#include "bat/confirmations/confirmation_type.h"
#include "bat/confirmations/internal/token_info.h"

#include "wrapper.hpp"  // NOLINT

using challenge_bypass_ristretto::BlindedToken;
using challenge_bypass_ristretto::Token;

namespace confirmations {

struct ConfirmationInfo {
  ConfirmationInfo();
  explicit ConfirmationInfo(const ConfirmationInfo& info);
  ~ConfirmationInfo();

  std::string id;
  std::string creative_instance_id;
  ConfirmationType type;
  TokenInfo token_info;
  Token payment_token;
  BlindedToken blinded_payment_token;
  std::string credential;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_CONFIRMATION_INFO_H_
