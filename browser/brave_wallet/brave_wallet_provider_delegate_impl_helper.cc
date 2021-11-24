/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl_helper.h"

#include <utility>

#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/browser/ui/brave_pages.h"
#include "chrome/browser/ui/browser_finder.h"
#include "content/public/browser/web_contents.h"

namespace {

base::OnceCallback<void()> g_NewSetupNeededForTestingCallback;

}  // namespace

namespace brave_wallet {

void ShowPanel(content::WebContents* web_contents) {
  if (!web_contents)
    return;

  auto* tab_helper =
      brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents);
  if (tab_helper)
    tab_helper->ShowBubble();
}

void ShowWalletOnboarding(content::WebContents* web_contents) {
  Browser* browser =
      web_contents ? chrome::FindBrowserWithWebContents(web_contents) : nullptr;

  if (browser) {
    brave::ShowBraveWalletOnboarding(browser);
  } else if (g_NewSetupNeededForTestingCallback) {
    std::move(g_NewSetupNeededForTestingCallback).Run();
  }
}

void SetCallbackForNewSetupNeededForTesting(
    base::OnceCallback<void()> callback) {
  g_NewSetupNeededForTestingCallback = std::move(callback);
}

}  // namespace brave_wallet
