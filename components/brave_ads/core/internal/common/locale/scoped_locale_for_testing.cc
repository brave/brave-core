/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/common/locale/scoped_locale_for_testing.h"

#include "brave/components/brave_ads/core/public/common/locale/locale_util.h"

namespace brave_ads::test {

ScopedCurrentLanguageCode::ScopedCurrentLanguageCode(
    const std::string& language_code) {
  last_language_code_ = MutableCurrentLanguageCodeForTesting();  // IN-TEST
  MutableCurrentLanguageCodeForTesting() = language_code;        // IN-TEST
}

ScopedCurrentLanguageCode::~ScopedCurrentLanguageCode() {
  Reset();
}

void ScopedCurrentLanguageCode::Reset() {
  MutableCurrentLanguageCodeForTesting() = last_language_code_;  // IN-TEST
}

void ScopedCurrentLanguageCode::Set(const std::string& language_code) {
  MutableCurrentLanguageCodeForTesting() = language_code;  // IN-TEST
}

ScopedCurrentCountryCode::ScopedCurrentCountryCode(
    const std::string& country_code) {
  last_country_code_ = MutableCurrentCountryCodeForTesting();  // IN-TEST
  MutableCurrentCountryCodeForTesting() = country_code;        // IN-TEST
}

ScopedCurrentCountryCode::~ScopedCurrentCountryCode() {
  Reset();
}

void ScopedCurrentCountryCode::Reset() {
  MutableCurrentLanguageCodeForTesting() = last_country_code_;  // IN-TEST
}

void ScopedCurrentCountryCode::Set(const std::string& country_code) {
  MutableCurrentLanguageCodeForTesting() = country_code;  // IN-TEST
}

}  // namespace brave_ads::test
