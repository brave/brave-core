/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_SECURITY_HELPER_H_
#define BAT_CONFIRMATIONS_SECURITY_HELPER_H_

#include <string>
#include <vector>

#include "brave/vendor/challenge_bypass_ristretto_ffi/src/wrapper.hpp"

namespace helper {

using namespace challenge_bypass_ristretto;

class Security {
 public:
  static std::string Sign(
      const std::vector<std::string>& keys,
      const std::vector<std::string>& values,
      const unsigned int size,
      const std::string& key_id,
      const std::vector<uint8_t>& public_key);

  static std::vector<Token> GenerateTokens(const unsigned int count);

  static std::vector<BlindedToken> BlindTokens(
      const std::vector<Token>& tokens);

  static std::vector<uint8_t> GetSHA256(const std::string& string);

  static std::string GetBase64(const std::vector<uint8_t>& data);
};

}  // namespace helper

#endif  // BAT_CONFIRMATIONS_SECURITY_HELPER_H_
