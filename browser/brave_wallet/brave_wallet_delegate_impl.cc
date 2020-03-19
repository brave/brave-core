/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_delegate_impl.h"

#include "brave/common/extensions/extension_constants.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/common/url_constants.h"

BraveWalletDelegateImpl::~BraveWalletDelegateImpl() {
}

void BraveWalletDelegateImpl::CloseTabsAndRestart() {
  // Close all CW tabs in each browser instance
  for (auto* browser : *BrowserList::GetInstance()) {
    auto* tab_strip = browser->tab_strip_model();
    for (int i = 0; i < tab_strip->count(); ++i) {
      auto* web_contents = tab_strip->GetWebContentsAt(i);
      GURL url = web_contents->GetURL();
      if (url.SchemeIs(content::kChromeUIScheme) &&
          url.host() == ethereum_remote_client_host) {
        web_contents->Close();
      }
    }
  }
  chrome::AttemptRestart();
}
