/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <regex>

#if defined (CHECK)
#undef CHECK
#endif
extern "C"
{
#include "relic.h"
}

namespace braveledger_bat_bignum {
void prepareBigNum(bn_t& big_num, const std::string& probi) {
  bn_null(big_num);
  bn_new(big_num);
  bn_read_str(big_num, probi.c_str(), probi.length(), 10);
}

bool isProbiValid(const std::string& probi) {
  // probi shouldn't be longer then 44
  if (probi.length() > 44) {
    return false;
  }

  // checks if probi only contains numbers
  return std::regex_match(probi, std::regex("^[0-9]*$"));
}

std::string bigNumToString(bn_t& number) {
  int result_length = bn_size_str(number, 10);
  const int MAX_BN_BUFF = 256; //global
  char result_char[MAX_BN_BUFF];
  bn_write_str(result_char, result_length, number, 10);
  std::string result_string(result_char);

  return result_string;
}

std::string sum(const std::string& a_string, const std::string& b_string) {
  if (!isProbiValid(a_string) || !isProbiValid(b_string)) {
    return "0";
  }

  bn_t a;
  bn_t b;
  bn_t result;

  prepareBigNum(a, a_string);
  prepareBigNum(b, b_string);

  bn_add(result, a, b);

  bn_free(a);
  bn_free(b);

  std::string result_str = bigNumToString(result);

  bn_free(result);
  return result_str;
}

std::string sub(const std::string& a_string, const std::string& b_string) {
  if (!isProbiValid(a_string) || !isProbiValid(b_string)) {
    return "0";
  }

  bn_t a;
  bn_t b;
  bn_t result;

  prepareBigNum(a, a_string);
  prepareBigNum(b, b_string);

  bn_sub(result, a, b);

  bn_free(a);
  bn_free(b);

  std::string result_str = bigNumToString(result);

  bn_free(result);
  return result_str;
}

std::string mul(const std::string& a_string, const std::string& b_string) {
  if (!isProbiValid(a_string) || !isProbiValid(b_string)) {
    return "0";
  }

  bn_t a;
  bn_t b;
  bn_t result;

  prepareBigNum(a, a_string);
  prepareBigNum(b, b_string);

  bn_mul_basic(result, a, b);

  bn_free(a);
  bn_free(b);

  std::string result_str = bigNumToString(result);

  bn_free(result);
  return result_str;
}

}  // namespace braveledger_bat_bignum
