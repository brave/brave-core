/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_UTIL_H_
#define BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_UTIL_H_

#include <string>

#include "url/gurl.h"

namespace de_amp {
bool MaybeFindCanonicalAmpUrl(const std::string& body,
                              std::string* canonical_url);
bool VerifyCanonicalAmpUrl(const GURL& canonical_url, const GURL& original_url);
}  // namespace de_amp

#endif  // BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_UTIL_H_
