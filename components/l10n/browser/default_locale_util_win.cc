/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/browser/default_locale_util.h"

#include <windows.h>

#include <optional>
#include <vector>

#include "base/strings/utf_string_conversions.h"

namespace brave_l10n {

std::optional<std::string> MaybeGetDefaultLocaleString() {
  const int buffer_size =
      ::GetLocaleInfoEx(/*lpLocaleName=*/nullptr,
                        /*LCType=*/LOCALE_SNAME, /*lpLCData=*/nullptr,
                        /*cchData=*/0);
  if (buffer_size == 0) {
    return std::nullopt;
  }

  std::vector<wchar_t> buffer(buffer_size);
  if (::GetLocaleInfoEx(/*lpLocaleName=*/nullptr,
                        /*LCType=*/LOCALE_SNAME,
                        /*lpLCData=*/buffer.data(),
                        /*cchData=*/buffer_size) == 0) {
    return std::nullopt;
  }

  return base::WideToUTF8(buffer.data());
}

}  // namespace brave_l10n
