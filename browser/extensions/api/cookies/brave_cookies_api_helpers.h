/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_COOKIES_BRAVE_COOKIES_API_HELPERS_H_
#define BRAVE_BROWSER_EXTENSIONS_API_COOKIES_BRAVE_COOKIES_API_HELPERS_H_

#include <string>
#include <vector>

#include "chrome/common/extensions/api/cookies.h"
#include "services/network/public/mojom/cookie_manager.mojom-forward.h"

namespace content {
class BrowserContext;
}

namespace extensions {
namespace brave_cookies_api_helpers {

// Resolves chrome.cookies storeId to a CookieManager, including Brave
// Containers stores ("containers:<uuid>").
network::mojom::CookieManager* ParseStoreCookieManager(
    content::BrowserContext* function_context,
    bool include_incognito,
    std::string* store_id,
    std::string* error);

// Builds cookie stores for getAllCookieStores(), grouping contained tabs under
// containers:<uuid> store ids.
std::vector<api::cookies::CookieStore> GetAllCookieStores(
    content::BrowserContext* browser_context,
    bool include_incognito);

}  // namespace brave_cookies_api_helpers
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_COOKIES_BRAVE_COOKIES_API_HELPERS_H_
