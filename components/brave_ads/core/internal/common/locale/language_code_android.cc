/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/locale/language_code.h"

#include <optional>
#include <string>
#include <string_view>

#include "base/android/locale_utils.h"

namespace brave_ads {

namespace {
constexpr std::string_view kDelimiters = "-_";
}  // namespace

std::optional<std::string> MaybeGetLanguageCodeString() {
  std::string locale = base::android::GetDefaultLocaleString();
  if (locale.empty()) {
    // Locale is not set.
    return std::nullopt;
  }

  const size_t pos = locale.find_first_of(kDelimiters);
  std::string language_code = locale.substr(0, pos);
  if (language_code.empty()) {
    return std::nullopt;
  }
  return language_code;
}

}  // namespace brave_ads
