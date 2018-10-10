/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_BIGNUM_H_
#define BRAVELEDGER_BAT_BIGNUM_H_

#include <string>

#if defined (CHECK)
#undef CHECK
#endif
extern "C"
{
#include "relic.h"
}

namespace braveledger_bat_bignum {
  void prepareBigNum(bn_t& result, const std::string& number);
  bool isProbiValid(const std::string& number);
  std::string bigNumToString(bn_t& number);
  std::string sum(const std::string& a_string, const std::string& b_string);
  std::string sub(const std::string& a_string, const std::string& b_string);
  std::string mul(const std::string& a_string, const std::string& b_string);
}  // namespace braveledger_bat_bignum

#endif  // BRAVELEDGER_BAT_BIGNUM_H_
