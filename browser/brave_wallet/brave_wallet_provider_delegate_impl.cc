/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl.h"
#include "brave/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "content/public/browser/web_contents.h"

namespace brave_wallet {

BraveWalletProviderDelegateImpl::BraveWalletProviderDelegateImpl(
    content::WebContents* web_contents)
    : web_contents_(web_contents) {}

void BraveWalletProviderDelegateImpl::ShowConnectToSiteUI() {
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents_);
  brave::ShowWalletBubble(browser);
}

}  // namespace brave_wallet
