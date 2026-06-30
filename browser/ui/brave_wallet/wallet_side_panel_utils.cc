/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_wallet/wallet_side_panel_utils.h"

#include "base/check.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "components/tabs/public/tab_interface.h"
#include "url/gurl.h"

namespace brave_wallet {

bool TryNavigateWalletSidePanel(content::WebContents* web_contents,
                                const GURL& url) {
  auto* tab = tabs::TabInterface::GetFromContents(web_contents);
  CHECK(tab);
  auto* browser = tab->GetBrowserWindowInterface();
  CHECK(browser);
  return browser->GetFeatures().NavigateWalletSidePanelIfActive(url);
}

}  // namespace brave_wallet
