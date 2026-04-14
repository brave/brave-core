/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_PAGES_H_
#define BRAVE_BROWSER_UI_BRAVE_PAGES_H_

#include <string_view>

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_talk/buildflags/buildflags.h"

class BrowserWindowInterface;

namespace brave {

void ShowBraveAdblock(BrowserWindowInterface* browser);
void ShowWebcompatReporter(BrowserWindowInterface* browser);
void ShowBraveRewards(BrowserWindowInterface* browser);
void ShowBraveWallet(BrowserWindowInterface* browser);
void ShowBraveWalletOnboarding(BrowserWindowInterface* browser);
void ShowBraveWalletAccountCreation(BrowserWindowInterface* browser,
                                    std::string_view coin_name);
void ShowExtensionSettings(BrowserWindowInterface* browser);
void ShowWalletSettings(BrowserWindowInterface* browser);
void ShowSync(BrowserWindowInterface* browser);
void ShowBraveNewsConfigure(BrowserWindowInterface* browser);
void ShowShortcutsPage(BrowserWindowInterface* browser);
#if BUILDFLAG(ENABLE_BRAVE_TALK)
void ShowBraveTalk(BrowserWindowInterface* browser);
#endif
#if BUILDFLAG(ENABLE_AI_CHAT)
void ShowFullpageChat(BrowserWindowInterface* browser);
#endif

void ShowAppsPage(BrowserWindowInterface* browser);

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_BRAVE_PAGES_H_
