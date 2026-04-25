/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/sys_string_conversions.h"
#include "brave/components/l10n/browser/default_locale_util.h"

#import <Foundation/Foundation.h>

#include <optional>

namespace brave_l10n {

std::optional<std::string> MaybeGetDefaultLocaleString() {
  NSLocale* locale = NSLocale.currentLocale;
  NSString* localeIdentifier = [NSLocale localeIdentifierFromComponents:@{
    NSLocaleLanguageCode : locale.languageCode,
    NSLocaleCountryCode : locale.countryCode ?: @"US"
  }];
  return base::SysNSStringToUTF8(localeIdentifier);
}

}  // namespace brave_l10n
