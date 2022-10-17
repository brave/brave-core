/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/time_profiler/pretty_function_parser_util.h"

#include "base/check.h"

namespace ads {

namespace {

constexpr char kOpenParenthisis[] = "(";

size_t FindStartOfName(const std::string& pretty_function) {
  DCHECK(!pretty_function.empty());

  size_t end_pos = pretty_function.find(kOpenParenthisis);
  if (end_pos == std::string::npos) {
    end_pos = pretty_function.length();
  }

  size_t start_pos = pretty_function.substr(0, end_pos).rfind(" ") + 1;
  if (start_pos == std::string::npos) {
    start_pos = 0;
  }

  return start_pos;
}

size_t FindEndOfName(const std::string& pretty_function) {
  DCHECK(!pretty_function.empty());

  size_t pos = pretty_function.find(kOpenParenthisis);
  if (pos == std::string::npos) {
    pos = pretty_function.length();
  }

  return pos;
}

}  // namespace

std::string ParseFunctionFromPrettyFunction(
    const std::string& pretty_function) {
  DCHECK(!pretty_function.empty());

  const size_t pos = FindStartOfName(pretty_function);
  const size_t length = FindEndOfName(pretty_function) - pos;
  return pretty_function.substr(pos, length);
}

}  // namespace ads
