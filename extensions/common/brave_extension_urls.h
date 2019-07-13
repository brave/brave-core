/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_EXTENSIONS_COMMON_EXTENSION_URLS_H_
#define BRAVE_EXTENSIONS_COMMON_EXTENSION_URLS_H_

#include "extensions/common/extension_urls.h"
#include "url/gurl.h"

namespace extension_urls {

class BraveProtectedUrls {
public:
  static bool IsHiddenNetworkRequest(const url::Origin& origin,
                                     base::StringPiece path);

  static std::vector<const GURL> ContentScriptWithheldUrls();

private:
  static const char* UpholdUrls[][2];
};

}  // namespace extension_urls

#endif  // BRAVE_EXTENSIONS_COMMON_EXTENSION_URLS_H_
