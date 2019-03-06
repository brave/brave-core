/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_LOCALE_HELPER_H_
#define BAT_ADS_INTERNAL_LOCALE_HELPER_H_

#include <string>

namespace helper {

class Locale {
 public:
  static const std::string GetLanguageCode(const std::string& locale);
  static const std::string GetCountryCode(const std::string& locale);
};

}  // namespace helper

#endif  // BAT_ADS_INTERNAL_LOCALE_HELPER_H_
