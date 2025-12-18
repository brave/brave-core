/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/search_query_metrics/locale/scoped_locale_for_testing.h"

#include "brave/components/search_query_metrics/locale/locale_util.h"

namespace metrics::test {

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

}  // namespace metrics::test
