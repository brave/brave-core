/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_LOCALE_TEST_FAKE_LOCALE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_LOCALE_TEST_FAKE_LOCALE_H_

#include <string>

#include "brave/components/brave_ads/core/internal/common/locale/locale.h"
#include "brave/components/brave_ads/core/public/common/locale/locale_util.h"

namespace brave_ads::test {

// A test double for `Locale` that installs itself as the active instance on
// construction and restores the default on destruction. Call `SetLanguageCode`
// or `SetCountryCode` to change the simulated locale at any point during a
// test.
class FakeLocale final : public Locale {
 public:
  FakeLocale();

  FakeLocale(const FakeLocale&) = delete;
  FakeLocale& operator=(const FakeLocale&) = delete;

  ~FakeLocale() override;

  void SetLanguageCode(const std::string& language_code);
  void SetCountryCode(const std::string& country_code);

  // Locale:
  std::string GetLanguageCode() const override;
  std::string GetCountryCode() const override;

 private:
  std::string language_code_ = kDefaultLanguageCode;
  std::string country_code_ = kDefaultCountryCode;
};

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_LOCALE_TEST_FAKE_LOCALE_H_
