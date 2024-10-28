/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_PAGES_H_
#define BRAVE_BROWSER_UI_BRAVE_PAGES_H_

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-forward.h"

class Browser;

namespace brave {

void ShowBraveAdblock(Browser* browser);
void ShowWebcompatReporter(Browser* browser);
void ShowBraveRewards(Browser* browser);
void ShowBraveWallet(Browser* browser);
void ShowBraveWalletOnboarding(Browser* browser);
void ShowBraveWalletAccountCreation(Browser* browser,
                                    brave_wallet::mojom::CoinType coin_type);
void ShowExtensionSettings(Browser* browser);
void ShowWalletSettings(Browser* browser);
void ShowSync(Browser* browser);
void ShowBraveNewsConfigure(Browser* browser);
void ShowShortcutsPage(Browser* browser);
void ShowBraveTalk(Browser* browser);

#if BUILDFLAG(ENABLE_AI_CHAT)
void ShowFullpageChat(Browser* browser);
#endif

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_BRAVE_PAGES_H_
