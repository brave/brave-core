/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_UTIL_H_
#define BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_UTIL_H_

#include <string>
#include <string_view>

#include "base/types/expected.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace de_amp {
// Check feature flag and user pref
bool IsDeAmpEnabled(PrefService* prefs);

// Run a regex against a string to check if AMP page
bool CheckIfAmpPage(std::string_view body);

// Find canonical link in body or return error
// Caller makes sure that body is AMP page
base::expected<std::string, std::string> FindCanonicalAmpUrl(
    std::string_view body);

// Validation check for canonical URL
bool VerifyCanonicalAmpUrl(const GURL& canonical_url, const GURL& original_url);
}  // namespace de_amp

#endif  // BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_UTIL_H_
