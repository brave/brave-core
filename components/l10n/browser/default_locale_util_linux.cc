/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cstdlib>

#include "brave/components/l10n/browser/default_locale_util.h"

namespace brave_l10n {

absl::optional<std::string> MaybeGetDefaultLocaleString() {
  // LC_ALL should always override the LANG variable, whether it is set or not.
  char const* language = language = std::getenv("LC_ALL");

  if (!language || !*language) {
    language = std::getenv("LANG");
  }

  if (!language || !*language) {
    return absl::nullopt;
  }

  return {language};
}

}  // namespace brave_l10n
