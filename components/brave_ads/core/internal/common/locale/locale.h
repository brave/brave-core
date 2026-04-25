/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_LOCALE_LOCALE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_LOCALE_LOCALE_H_

#include <string>

namespace brave_ads {

// Provides the device locale (language and country codes). Tests may inject a
// controlled value via SetForTesting to decouple locale-sensitive logic from
// the real device locale.
class Locale {
 public:
  static const Locale& GetInstance();

  static void SetForTesting(const Locale* locale);

  Locale();

  Locale(const Locale&) = delete;
  Locale& operator=(const Locale&) = delete;

  virtual ~Locale();

  virtual std::string GetLanguageCode() const;
  virtual std::string GetCountryCode() const;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_LOCALE_LOCALE_H_
