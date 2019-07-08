/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_SECURITY_HELPER_H_
#define BAT_CONFIRMATIONS_INTERNAL_SECURITY_HELPER_H_

#include <string>
#include <vector>
#include <map>

#include "wrapper.hpp"

using challenge_bypass_ristretto::Token;
using challenge_bypass_ristretto::BlindedToken;

namespace helper {

class Security {
 public:
  static std::string Sign(
      const std::map<std::string, std::string>& headers,
      const std::string& key_id,
      const std::vector<uint8_t>& private_key);

  static std::vector<Token> GenerateTokens(const int count);

  static std::vector<BlindedToken> BlindTokens(
      const std::vector<Token>& tokens);

  static std::vector<uint8_t> GetSHA256(const std::string& string);

  static std::string GetBase64(const std::vector<uint8_t>& data);
};

}  // namespace helper

#endif  // BAT_CONFIRMATIONS_INTERNAL_SECURITY_HELPER_H_
