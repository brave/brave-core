/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_L10N_BROWSER_LOCALE_HELPER_MAC_H_
#define BRAVE_COMPONENTS_L10N_BROWSER_LOCALE_HELPER_MAC_H_

#include <string>

#include "brave/components/l10n/browser/locale_helper.h"

namespace brave_l10n {

class LocaleHelperMac : public LocaleHelper {
 public:
  LocaleHelperMac(const LocaleHelperMac&) = delete;
  LocaleHelperMac& operator=(const LocaleHelperMac&) = delete;

  static LocaleHelperMac* GetInstanceImpl();

 private:
  friend struct base::DefaultSingletonTraits<LocaleHelperMac>;

  LocaleHelperMac();
  ~LocaleHelperMac() override;

  // LocaleHelper impl
  std::string GetLocale() const override;
};

}  // namespace brave_l10n

#endif  // BRAVE_COMPONENTS_L10N_BROWSER_LOCALE_HELPER_MAC_H_
