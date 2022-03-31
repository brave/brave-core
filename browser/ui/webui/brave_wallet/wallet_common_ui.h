/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_COMMON_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_COMMON_UI_H_

#include <stdint.h>

class Profile;
class Browser;

namespace url {
class Origin;
}  // namespace url

namespace content {
class WebContents;
}  // namespace content

namespace brave_wallet {

void AddBlockchainTokenImageSource(Profile* profile);

bool IsBraveWalletOrigin(const url::Origin& origin);

content::WebContents* GetWebContentsFromTabId(Browser** browser,
                                              int32_t tab_id);
content::WebContents* GetActiveWebContents();

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_COMMON_UI_H_
