/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/search_query_metrics/locale/locale_util.h"

#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "brave/components/search_query_metrics/locale/language_code.h"

namespace metrics {

namespace {

std::string& MutableCurrentLanguageCode() {
  // ISO 639-1 language code (e.g. "en", "fr", "de").
  static base::NoDestructor<std::string> language_code(base::ToLowerASCII(
      MaybeGetLanguageCodeString().value_or(kDefaultLanguageCode)));
  return *language_code;
}

}  // namespace

const std::string& CurrentLanguageCode() {
  return MutableCurrentLanguageCode();
}

std::string& MutableCurrentLanguageCodeForTesting() {
  return MutableCurrentLanguageCode();
}

}  // namespace metrics
