/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_TOKEN_INFO_H_
#define BAT_CONFIRMATIONS_INTERNAL_TOKEN_INFO_H_

#include <string>

#include "wrapper.hpp"  // NOLINT

using challenge_bypass_ristretto::UnblindedToken;

namespace confirmations {

struct TokenInfo {
  TokenInfo();
  TokenInfo(
      const TokenInfo& info);
  ~TokenInfo();

  UnblindedToken unblinded_token;
  std::string public_key;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_TOKEN_INFO_H_
