/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/browser/default_locale_util.h"

#include <optional>

#include "base/i18n/android_locale.h"

namespace brave_l10n {

std::optional<std::string> MaybeGetDefaultLocaleString() {
  return std::string(base::i18n::GetAndroidDefaultLocale().tag_string());
}

}  // namespace brave_l10n
