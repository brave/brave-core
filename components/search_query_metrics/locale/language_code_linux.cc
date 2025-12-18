/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/search_query_metrics/locale/language_code.h"

#include <locale.h>

#include <string_view>

namespace metrics {

namespace {
constexpr std::string_view kDelimiters = "-_";
}  // namespace

std::optional<std::string> MaybeGetLanguageCodeString() {
  const char* const locale = setlocale(LC_MESSAGES, nullptr);
  if (!locale) {
    // Locale is not set.
    return std::nullopt;
  }
  std::string locale_string(locale);

  const size_t pos = locale_string.find_first_of(kDelimiters);
  std::string language_code = locale_string.substr(0, pos);
  if (language_code.empty() || language_code == "C" ||
      language_code == "POSIX") {
    // Standard C locale.
    return std::nullopt;
  }
  return language_code;
}

}  // namespace metrics
