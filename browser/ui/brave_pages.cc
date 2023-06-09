/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_pages.h"

#include "base/strings/strcat.h"
#include "brave/browser/ui/webui/webcompat_reporter/webcompat_reporter_dialog.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/sidebar/constants.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/common/webui_url_constants.h"
#include "url/gurl.h"

namespace brave {

void ShowBraveRewards(Browser* browser) {
  NavigateParams params(
      GetSingletonTabNavigateParams(browser, GURL(kBraveUIRewardsURL)));
  ShowSingletonTabOverwritingNTP(browser, &params);
}

void ShowBraveAdblock(Browser* browser) {
  NavigateParams params(
      GetSingletonTabNavigateParams(browser, GURL(kBraveUIAdblockURL)));
  ShowSingletonTabOverwritingNTP(browser, &params);
}

void ShowSync(Browser* browser) {
  auto url = chrome::GetSettingsUrl(chrome::kSyncSetupSubPage);
  NavigateParams params(GetSingletonTabNavigateParams(browser, url));
  ShowSingletonTabOverwritingNTP(browser, &params);
}

void ShowBraveNewsConfigure(Browser* browser) {
  NavigateParams params(GetSingletonTabNavigateParams(
      browser, GURL("brave://newtab/?openSettings=BraveNews")));
  ShowSingletonTabOverwritingNTP(browser, &params);
}

void ShowShortcutsPage(Browser* browser) {
  NavigateParams params(
      GetSingletonTabNavigateParams(browser, GURL(kShortcutsURL)));
  ShowSingletonTabOverwritingNTP(browser, &params);
}

void ShowBraveTalk(Browser* browser) {
  NavigateParams params(
      GetSingletonTabNavigateParams(browser, GURL(sidebar::kBraveTalkURL)));
  ShowSingletonTabOverwritingNTP(browser, &params);
}

void ShowWebcompatReporter(Browser* browser) {
  content::WebContents* web_contents =
      browser->tab_strip_model()->GetActiveWebContents();
  if (!web_contents) {
    return;
  }

  OpenWebcompatReporterDialog(web_contents);
}

void ShowBraveWallet(Browser* browser) {
  NavigateParams params(
      GetSingletonTabNavigateParams(browser, GURL(kBraveUIWalletURL)));
  ShowSingletonTabOverwritingNTP(browser, &params);
}

void ShowBraveWalletOnboarding(Browser* browser) {
  NavigateParams params(GetSingletonTabNavigateParams(
      browser, GURL(kBraveUIWalletOnboardingURL)));
  ShowSingletonTabOverwritingNTP(browser, &params);
}

void ShowBraveWalletAccountCreation(Browser* browser,
                                    brave_wallet::mojom::CoinType coin_type) {
  // Only solana is supported.
  if (coin_type == brave_wallet::mojom::CoinType::SOL) {
    NavigateParams params(GetSingletonTabNavigateParams(
        browser,
        GURL(base::StrCat({kBraveUIWalletAccountCreationURL, "Solana"}))));
    ShowSingletonTabOverwritingNTP(browser, &params);
  } else {
    NOTREACHED();
  }
}

void ShowExtensionSettings(Browser* browser) {
  NavigateParams params(
      GetSingletonTabNavigateParams(browser, GURL(kExtensionSettingsURL)));
  ShowSingletonTabOverwritingNTP(browser, &params);
}

void ShowWalletSettings(Browser* browser) {
  NavigateParams params(
      GetSingletonTabNavigateParams(browser, GURL(kWalletSettingsURL)));
  ShowSingletonTabOverwritingNTP(browser, &params);
}

void ShowIPFS(Browser* browser) {
  NavigateParams params(
      GetSingletonTabNavigateParams(browser, GURL(kIPFSWebUIURL)));
  ShowSingletonTabOverwritingNTP(browser, &params);
}

}  // namespace brave
