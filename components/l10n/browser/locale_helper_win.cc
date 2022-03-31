/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/browser/locale_helper_win.h"

#include <windows.h>

#include <memory>

namespace brave_l10n {

LocaleHelperWin::LocaleHelperWin() = default;

LocaleHelperWin::~LocaleHelperWin() = default;

std::string LocaleHelperWin::GetLocale() const {
  auto size = ::GetLocaleInfoEx(nullptr, LOCALE_SNAME, nullptr, 0);
  if (size == 0) {
    return kDefaultLocale;
  }

  std::unique_ptr<wchar_t[]>locale_name(new wchar_t[size]);
  if (!locale_name.get()) {
    return kDefaultLocale;
  }

  auto ret = ::GetLocaleInfoEx(nullptr, LOCALE_SNAME, locale_name.get(), size);
  if (ret == 0) {
    return kDefaultLocale;
  }

  std::unique_ptr<char[]>locale(new char[size]);
  if (!locale.get()) {
    return kDefaultLocale;
  }

  wcstombs(locale.get(), locale_name.get(), size);

  return std::string(locale.get());
}

LocaleHelperWin* LocaleHelperWin::GetInstanceImpl() {
  return base::Singleton<LocaleHelperWin>::get();
}

LocaleHelper* LocaleHelper::GetInstanceImpl() {
  return LocaleHelperWin::GetInstanceImpl();
}

}  // namespace brave_l10n
