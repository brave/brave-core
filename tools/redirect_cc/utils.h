
/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_TOOLS_REDIRECT_CC_UTILS_H_
#define BRAVE_TOOLS_REDIRECT_CC_UTILS_H_

#include <string>
#include <vector>

#include "brave/tools/redirect_cc/types.h"

namespace utils {

bool StartsWith(FilePathStringView str, FilePathStringView search_for);
bool EndsWith(FilePathStringView str, FilePathStringView search_for);

template <typename T, typename CharT = typename T::value_type>
static std::vector<T> SplitString(T str, T delimiter) {
  std::vector<T> result;
  if (str.empty())
    return result;

  size_t start = 0;
  while (start != std::basic_string<CharT>::npos) {
    size_t end = str.find_first_of(delimiter, start);

    std::basic_string_view<CharT> piece;
    if (end == std::basic_string<CharT>::npos) {
      piece = str.substr(start);
      start = std::basic_string<CharT>::npos;
    } else {
      piece = str.substr(start, end - start);
      start = end + 1;
    }

    result.emplace_back(piece);
  }
  return result;
}

}  // namespace utils

#endif  // BRAVE_TOOLS_REDIRECT_CC_UTILS_H_
