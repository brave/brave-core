/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/search_query_metrics/locale/language_code.h"

#include <windows.h>

#include <vector>

#include "base/strings/utf_string_conversions.h"

namespace metrics {

std::optional<std::string> MaybeGetLanguageCodeString() {
  const int buffer_size =
      ::GetLocaleInfoEx(/*lpLocaleName=*/nullptr,
                        /*LCType=*/LOCALE_SISO639LANGNAME, /*lpLCData=*/nullptr,
                        /*cchData=*/0);
  if (buffer_size == 0) {
    return std::nullopt;
  }

  std::vector<wchar_t> buffer(buffer_size);
  if (::GetLocaleInfoEx(
          /*lpLocaleName=*/nullptr, /*LCType=*/LOCALE_SISO639LANGNAME,
          /*lpLCData=*/buffer.data(), /*cchData=*/buffer_size) == 0) {
    return std::nullopt;
  }

  return base::WideToUTF8(buffer.data());
}

}  // namespace metrics
