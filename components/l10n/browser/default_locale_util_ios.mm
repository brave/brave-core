/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/browser/default_locale_util.h"

#import <Foundation/Foundation.h>

#include <optional>

namespace brave_l10n {

std::optional<std::string> MaybeGetDefaultLocaleString() {
  const NSLocale* const locale = NSLocale.currentLocale;
  return [NSString
             stringWithFormat:@"%@_%@", locale.languageCode, locale.countryCode]
      .UTF8String;
}

}  // namespace brave_l10n
