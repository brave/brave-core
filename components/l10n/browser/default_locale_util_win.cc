/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/browser/default_locale_util.h"

#include <windows.h>

#include <memory>
#include <optional>

namespace brave_l10n {

std::optional<std::string> MaybeGetDefaultLocaleString() {
  const int buffer_size =
      ::GetLocaleInfoEx(/*lpLocaleName*/ nullptr,
                        /*LCType*/ LOCALE_SNAME, /*lpLCData*/ nullptr,
                        /*cchData*/ 0);
  if (buffer_size == 0) {
    return std::nullopt;
  }

  const std::unique_ptr<wchar_t[]> locale_sname(new wchar_t[buffer_size]);
  if (::GetLocaleInfoEx(/*lpLocaleName*/ nullptr, /*LCType*/ LOCALE_SNAME,
                        /*lpLCData*/ locale_sname.get(),
                        /*cchData*/ buffer_size) == 0) {
    return std::nullopt;
  }

  const std::unique_ptr<char[]> default_locale(new char[buffer_size]);
  wcstombs(/*dest*/ default_locale.get(), /*src*/ locale_sname.get(),
           /*max*/ buffer_size);
  return {default_locale.get()};
}

}  // namespace brave_l10n
