// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_wallet/page_handler/wallet_page_handler.h"

#include <utility>

#include "brave/browser/ui/browser_commands.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"

#if defined(TOOLKIT_VIEWS)
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#endif

WalletPageHandler::WalletPageHandler(
    mojo::PendingReceiver<brave_wallet::mojom::PageHandler> receiver,
    Profile* profile)
    : profile_(profile),
      receiver_(this, std::move(receiver)),
      weak_ptr_factory_(this) {}

WalletPageHandler::~WalletPageHandler() = default;

void WalletPageHandler::ShowApprovePanelUI() {
#if defined(TOOLKIT_VIEWS)
  Browser* browser = chrome::FindBrowserWithProfile(profile_);
  if (browser) {
    brave::ShowApproveWalletBubble(browser);
  }
#endif
}
