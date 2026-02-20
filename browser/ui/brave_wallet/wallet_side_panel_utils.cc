/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_wallet/wallet_side_panel_utils.h"

#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "url/gurl.h"

namespace brave_wallet {

bool TryNavigateWalletSidePanel(content::WebContents* web_contents,
                                const GURL& url) {
  for (auto* bwi : GetAllBrowserWindowInterfaces()) {
    if (bwi->GetTabStripModel()->GetIndexOfWebContents(web_contents) !=
        TabStripModel::kNoTab) {
      return bwi->GetFeatures().NavigateWalletSidePanelIfActive(url);
    }
  }
  return false;
}

bool IsWebContentsActive(content::WebContents& web_contents) {
  if (auto* bwi = GetLastActiveBrowserWindowInterfaceWithAnyProfile()) {
    return bwi->GetTabStripModel()->GetActiveWebContents() == &web_contents;
  }
  return false;
}

}  // namespace brave_wallet
