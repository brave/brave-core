/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_INFO_H_

#include <string>

#include "base/time/time.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_token_info.h"
#include "wrapper.hpp"

namespace ads {

using challenge_bypass_ristretto::BlindedToken;
using challenge_bypass_ristretto::Token;

struct ConfirmationInfo final {
  ConfirmationInfo();
  ConfirmationInfo(const ConfirmationInfo& info);
  ~ConfirmationInfo();

  bool operator==(const ConfirmationInfo& rhs) const;
  bool operator!=(const ConfirmationInfo& rhs) const;

  bool IsValid() const;

  std::string id;
  std::string creative_instance_id;
  ConfirmationType type = ConfirmationType::kUndefined;
  privacy::UnblindedTokenInfo unblinded_token;
  Token payment_token;
  BlindedToken blinded_payment_token;
  std::string credential;
  std::string user_data;
  base::Time created_at;
  bool was_created = false;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_INFO_H_
