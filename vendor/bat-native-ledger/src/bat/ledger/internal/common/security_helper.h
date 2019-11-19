/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_COMMON_SECURITY_HELPER_H_
#define BRAVELEDGER_COMMON_SECURITY_HELPER_H_

#include <map>
#include <string>
#include <vector>

#include "wrapper.hpp"

using challenge_bypass_ristretto::Token;
using challenge_bypass_ristretto::BlindedToken;

namespace braveledger_helper {

class Security {
 public:
  static std::vector<Token> GenerateTokens(const int count);

  static std::vector<BlindedToken> BlindTokens(
      const std::vector<Token>& tokens);

  static std::vector<uint8_t> GetSHA256(const std::string& string);

  static std::string GetBase64(const std::vector<uint8_t>& data);

  static std::string Sign(
      const std::vector<std::map<std::string, std::string>>& headers,
      const std::string& key_id,
      const std::vector<uint8_t>& private_key);
};

}  // namespace braveledger_helper

#endif  // BRAVELEDGER_COMMON_SECURITY_HELPER_H_
