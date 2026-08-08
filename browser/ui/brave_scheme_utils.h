// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_BRAVE_SCHEME_UTILS_H_
#define BRAVE_BROWSER_UI_BRAVE_SCHEME_UTILS_H_

#include <string>

class GURL;

namespace brave_utils {
// Replaces the chrome:// scheme with brave:// scheme in the given |url_string|.
bool ReplaceChromeToBraveScheme(std::u16string& url_string);

// Same, for a GURL. Returns |url| unchanged if it is not chrome://.
GURL ReplaceChromeToBraveScheme(const GURL& url);
}  // namespace brave_utils

#endif  // BRAVE_BROWSER_UI_BRAVE_SCHEME_UTILS_H_
