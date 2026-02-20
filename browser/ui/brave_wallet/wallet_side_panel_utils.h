/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_WALLET_WALLET_SIDE_PANEL_UTILS_H_
#define BRAVE_BROWSER_UI_BRAVE_WALLET_WALLET_SIDE_PANEL_UTILS_H_

class GURL;

namespace content {
class WebContents;
}  // namespace content

namespace brave_wallet {

// Returns true if the wallet side panel is currently visible in the browser
// window that owns |web_contents| and successfully navigates it to |url|.
bool TryNavigateWalletSidePanel(content::WebContents* web_contents,
                                const GURL& url);

// Returns true if |web_contents| is the active tab in the most recently
// focused browser window.
bool IsWebContentsActive(content::WebContents& web_contents);

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_UI_BRAVE_WALLET_WALLET_SIDE_PANEL_UTILS_H_
