/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_L10N_BROWSER_DEFAULT_LOCALE_UTIL_H_
#define BRAVE_COMPONENTS_L10N_BROWSER_DEFAULT_LOCALE_UTIL_H_

#include <string>

#include "absl/types/optional.h"

namespace brave_l10n {

// Returns the current default locale of the device. When the locale should
// match the application locale or an eligible string pack for localization use
// the canonicalized l10n_util::GetApplicationLocale.
absl::optional<std::string> MaybeGetDefaultLocaleString();

}  // namespace brave_l10n

#endif  // BRAVE_COMPONENTS_L10N_BROWSER_DEFAULT_LOCALE_UTIL_H_
