// Copyright (c) 2026 The BNES Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BASE_STRINGS_STRING_UTIL_H_
#define BASE_STRINGS_STRING_UTIL_H_

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace base {

inline std::string ToLowerASCII(const std::string& str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return result;
}

inline bool EndsWith(std::string_view str, std::string_view suffix,
                     int case_sensitive = 0) {
  if (str.size() < suffix.size()) {
    return false;
  }
  if (case_sensitive) {
    return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
  }
  return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin(),
                    [](char a, char b) {
                      return std::tolower(static_cast<unsigned char>(a)) ==
                             std::tolower(static_cast<unsigned char>(b));
                    });
}

inline bool IsAsciiAlphaNumeric(char character) {
  return std::isalnum(static_cast<unsigned char>(character)) != 0;
}

inline bool EqualsCaseInsensitiveASCII(std::string_view a,
                                       std::string_view b) {
  return std::equal(a.begin(), a.end(), b.begin(),
                    [](char x, char y) {
                      return std::tolower(static_cast<unsigned char>(x)) ==
                             std::tolower(static_cast<unsigned char>(y));
                    });
}

enum CompareCase { SENSITIVE };

}  // namespace base

#endif  // BASE_STRINGS_STRING_UTIL_H_
