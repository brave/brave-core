/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_UTILS_H_
#define BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_UTILS_H_

class Browser;
class GURL;

namespace sidebar {

class SidebarService;

bool CanUseSidebar(Browser* browser);
bool CanAddCurrentActiveTabToSidebar(Browser* browser);

// Exported for testing.
bool CanUseNotAddedBuiltInItemInsteadOf(SidebarService* service,
                                        const GURL& url);
GURL ConvertURLToBuiltInItemURL(const GURL& url);

}  // namespace sidebar

#endif  // BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_UTILS_H_
