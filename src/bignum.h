/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_BIGNUM_H_
#define BRAVELEDGER_BAT_BIGNUM_H_

#include <string>

namespace braveledger_bat_bignum {
  std::string sum(const std::string& a_string, const std::string& b_string);
  std::string sub(const std::string& a_string, const std::string& b_string);
  std::string mul(const std::string& a_string, const std::string& b_string);
}  // namespace braveledger_bat_bignum

#endif  // BRAVELEDGER_BAT_BIGNUM_H_
