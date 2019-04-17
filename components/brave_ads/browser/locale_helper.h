/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BROWSER_BRAVE_ADS_LOCALE_HELPER_H_
#define BRAVE_COMPONENTS_BROWSER_BRAVE_ADS_LOCALE_HELPER_H_

#include <string>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "build/build_config.h"

namespace brave_ads {

const char kDefaultLocale[] = "en-cUS";

class LocaleHelper {
 public:
  static LocaleHelper* GetInstance();

  // Should return the language based upon the tagging conventions of RFC 4646
  virtual std::string GetLocale() const;

 protected:
  LocaleHelper();
  virtual ~LocaleHelper();

 private:
  friend struct base::DefaultSingletonTraits<LocaleHelper>;

  DISALLOW_COPY_AND_ASSIGN(LocaleHelper);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BROWSER_BRAVE_ADS_LOCALE_HELPER_H_
