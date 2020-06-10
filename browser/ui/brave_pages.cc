/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_pages.h"

#include "brave/browser/webcompat_reporter/webcompat_reporter_dialog.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/common/webui_url_constants.h"
#include "url/gurl.h"

namespace brave {

void ShowBraveRewards(Browser* browser) {
  ShowSingletonTabOverwritingNTP(
      browser,
      GetSingletonTabNavigateParams(browser, GURL(kBraveUIRewardsURL)));
}

void ShowBraveAdblock(Browser* browser) {
  ShowSingletonTabOverwritingNTP(
      browser,
      GetSingletonTabNavigateParams(browser, GURL(kBraveUIAdblockURL)));
}

void ShowSync(Browser* browser) {
  auto url = chrome::GetSettingsUrl(chrome::kSyncSetupSubPage);
  ShowSingletonTabOverwritingNTP(
      browser,
      GetSingletonTabNavigateParams(browser, url));
}

void ShowWebcompatReporter(Browser* browser) {
  content::WebContents* web_contents =
      browser->tab_strip_model()->GetActiveWebContents();
  if (!web_contents)
    return;

  OpenWebcompatReporterDialog(web_contents);
}

void ShowBraveWallet(Browser* browser) {
  ShowSingletonTabOverwritingNTP(
      browser,
      GetSingletonTabNavigateParams(browser, GURL(kBraveUIWalletURL)));
}

void ShowExtensionSettings(Browser* browser) {
  ShowSingletonTabOverwritingNTP(
      browser,
      GetSingletonTabNavigateParams(browser, GURL(kExtensionSettingsURL)));
}

}  // namespace brave
