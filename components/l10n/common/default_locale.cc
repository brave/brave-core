/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/common/default_locale.h"

#include "base/no_destructor.h"
#include "brave/components/l10n/browser/default_locale_util.h"

namespace brave_l10n {

namespace {

constexpr char kFallbackLocale[] = "en_US";

std::string& MutableDefaultLocaleString() {
  static base::NoDestructor<std::string> locale(
      MaybeGetDefaultLocaleString().value_or(kFallbackLocale));
  return *locale;
}

}  // namespace

const std::string& DefaultLocaleString() {
  return MutableDefaultLocaleString();
}

std::string& MutableDefaultLocaleStringForTesting() {
  return MutableDefaultLocaleString();
}

}  // namespace brave_l10n
