/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_LOCALE_HELPER_LINUX_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_LOCALE_HELPER_LINUX_H_

#include <string>

#include "brave/components/brave_ads/browser/locale_helper.h"

namespace brave_ads {

class LocaleHelperLinux : public LocaleHelper {
 public:
  LocaleHelperLinux();
  ~LocaleHelperLinux() override;

  static LocaleHelperLinux* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<LocaleHelperLinux>;

  // LocaleHelper impl
  std::string GetLocale() const override;

  DISALLOW_COPY_AND_ASSIGN(LocaleHelperLinux);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_LOCALE_HELPER_LINUX_H_
