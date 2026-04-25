/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/locale/test/fake_locale.h"

namespace brave_ads::test {

FakeLocale::FakeLocale() {
  Locale::SetForTesting(this);
}

FakeLocale::~FakeLocale() {
  Locale::SetForTesting(nullptr);
}

void FakeLocale::SetLanguageCode(const std::string& language_code) {
  language_code_ = language_code;
}

void FakeLocale::SetCountryCode(const std::string& country_code) {
  country_code_ = country_code;
}

std::string FakeLocale::GetLanguageCode() const {
  return language_code_;
}

std::string FakeLocale::GetCountryCode() const {
  return country_code_;
}

}  // namespace brave_ads::test
