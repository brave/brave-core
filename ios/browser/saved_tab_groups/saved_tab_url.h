// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_SAVED_TAB_GROUPS_SAVED_TAB_URL_H_
#define BRAVE_IOS_BROWSER_SAVED_TAB_GROUPS_SAVED_TAB_URL_H_

#include <string>
#include <utility>

#include "url/gurl.h"

namespace brave {

// The URL and title to store for a tab. Unwraps reader mode and other
// `internal://` pages, maps the Brave new-tab page to `chrome://newtab`, and
// replaces anything saved tab groups cannot store with the default URL and
// title.
std::pair<GURL, std::u16string> NormalizeURLForSavedTabGroup(
    const GURL& url,
    const std::u16string& title);

}  // namespace brave

#endif  // BRAVE_IOS_BROWSER_SAVED_TAB_GROUPS_SAVED_TAB_URL_H_
