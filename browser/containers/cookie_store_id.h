/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_CONTAINERS_COOKIE_STORE_ID_H_
#define BRAVE_BROWSER_CONTAINERS_COOKIE_STORE_ID_H_

#include <optional>
#include <string>
#include <string_view>

namespace content {
class WebContents;
}

namespace containers {

// Builds a chrome.cookies storeId for a container partition:
// "<partition_domain>:<partition_name>" → "containers:<uuid>".
std::string GetContainerStoreId(std::string_view container_id);

// Returns the container UUID when |store_id| is a container store id; otherwise
// nullopt.
std::optional<std::string> ParseContainerStoreId(std::string_view store_id);

// True when |store_id| identifies a Brave Containers cookie store.
bool IsContainerStoreId(std::string_view store_id);

// Returns "containers:<uuid>" for a contained tab, otherwise the profile store
// id ("0" or "1").
std::string GetStoreIdForWebContents(content::WebContents* web_contents);

}  // namespace containers

#endif  // BRAVE_BROWSER_CONTAINERS_COOKIE_STORE_ID_H_
