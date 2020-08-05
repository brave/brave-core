/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CONFIRMATION_CONFIRMATION_INFO_H_
#define BAT_ADS_INTERNAL_CONFIRMATION_CONFIRMATION_INFO_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "wrapper.hpp"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_token_info.h"

namespace ads {

using challenge_bypass_ristretto::Token;
using challenge_bypass_ristretto::BlindedToken;

struct ConfirmationInfo {
  ConfirmationInfo();
  ConfirmationInfo(
      const ConfirmationInfo& info);
  ~ConfirmationInfo();

  bool operator==(
      const ConfirmationInfo& rhs) const;
  bool operator!=(
      const ConfirmationInfo& rhs) const;

  bool IsValid() const;

  std::string id;
  std::string creative_instance_id;
  ConfirmationType type = ConfirmationType::kNone;
  privacy::UnblindedTokenInfo unblinded_token;
  Token payment_token;
  BlindedToken blinded_payment_token;
  std::string credential;
  uint64_t timestamp_in_seconds = 0;
  bool created = false;
};

using ConfirmationList = std::vector<ConfirmationInfo>;

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CONFIRMATION_CONFIRMATION_INFO_H_
