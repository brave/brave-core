/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_pages.h"

#include <string_view>

#include "base/strings/strcat.h"
#include "brave/browser/ui/webui/webcompat_reporter/webcompat_reporter_dialog.h"
#include "brave/components/brave_talk/buildflags/buildflags.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/common/webui_url_constants.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/features.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_TALK)
#include "brave/components/sidebar/browser/constants.h"
#endif

namespace brave {

void ShowBraveRewards(BrowserWindowInterface* browser) {
  ShowSingletonTabOverwritingNTP(browser, GURL(kRewardsPageURL));
}

void ShowBraveAdblock(BrowserWindowInterface* browser) {
  ShowSingletonTabOverwritingNTP(browser, GURL(kBraveUIAdblockURL));
}

void ShowSync(BrowserWindowInterface* browser) {
  ShowSingletonTabOverwritingNTP(
      browser, chrome::GetSettingsUrl(chrome::kSyncSetupSubPage));
}

void ShowBraveNewsConfigure(BrowserWindowInterface* browser) {
  ShowSingletonTabOverwritingNTP(
      browser, GURL("brave://newtab/?openSettings=BraveNews"));
}

void ShowShortcutsPage(BrowserWindowInterface* browser) {
  ShowSingletonTabOverwritingNTP(browser, GURL(kShortcutsURL));
}

#if BUILDFLAG(ENABLE_BRAVE_TALK)
void ShowBraveTalk(BrowserWindowInterface* browser) {
  ShowSingletonTabOverwritingNTP(browser, GURL(sidebar::kBraveTalkURL));
}
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
void ShowFullpageChat(BrowserWindowInterface* browser) {
  if (!ai_chat::features::IsAIChatHistoryEnabled()) {
    return;
  }
  ShowSingletonTabOverwritingNTP(browser, GURL(kAIChatUIURL));
}
#endif

void ShowWebcompatReporter(BrowserWindowInterface* browser) {
  content::WebContents* web_contents =
      browser->GetTabStripModel()->GetActiveWebContents();
  if (!web_contents) {
    return;
  }

  webcompat_reporter::OpenReporterDialog(
      web_contents, webcompat_reporter::UISource::kAppMenu);
}

void ShowBraveWallet(BrowserWindowInterface* browser) {
  ShowSingletonTabOverwritingNTP(browser, GURL(kBraveUIWalletURL));
}

void ShowBraveWalletOnboarding(BrowserWindowInterface* browser) {
  ShowSingletonTabOverwritingNTP(browser, GURL(kBraveUIWalletOnboardingURL));
}

void ShowBraveWalletAccountCreation(BrowserWindowInterface* browser,
                                    std::string_view coin_name) {
  ShowSingletonTabOverwritingNTP(
      browser,
      GURL(base::StrCat({kBraveUIWalletAccountCreationURL, coin_name})));
}

void ShowExtensionSettings(BrowserWindowInterface* browser) {
  ShowSingletonTabOverwritingNTP(browser, GURL(kExtensionSettingsURL));
}

void ShowWalletSettings(BrowserWindowInterface* browser) {
  ShowSingletonTabOverwritingNTP(browser, GURL(kWalletSettingsURL));
}

void ShowAppsPage(BrowserWindowInterface* browser) {
  ShowSingletonTabOverwritingNTP(browser, GURL(chrome::kChromeUIAppsURL));
}

}  // namespace brave
