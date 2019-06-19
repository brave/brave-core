/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_EXTENSIONS_COMMON_EXTENSION_URLS_H_
#define BRAVE_EXTENSIONS_COMMON_EXTENSION_URLS_H_

#include "extensions/common/extension_urls.h"

namespace extension_urls {

// Returns true if the URL points to a security-critical service.
bool IsBraveProtectedUrl(const url::Origin& origin, base::StringPiece path);

}  // namespace extension_urls

#endif  // BRAVE_EXTENSIONS_COMMON_EXTENSION_URLS_H_
