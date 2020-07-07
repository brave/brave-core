/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/browser/locale_helper_mac.h"

#import <Cocoa/Cocoa.h>

namespace brave_l10n {

LocaleHelperMac::LocaleHelperMac() = default;

LocaleHelperMac::~LocaleHelperMac() = default;

std::string LocaleHelperMac::GetLocale() const {
  NSString *locale = [[NSLocale preferredLanguages] firstObject];
  return locale.UTF8String;
}

LocaleHelperMac* LocaleHelperMac::GetInstanceImpl() {
  return base::Singleton<LocaleHelperMac>::get();
}

LocaleHelper* LocaleHelper::GetInstanceImpl() {
  return LocaleHelperMac::GetInstanceImpl();
}

}  // namespace brave_l10n
