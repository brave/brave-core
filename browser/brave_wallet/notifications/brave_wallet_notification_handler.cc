/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/notifications/brave_wallet_notification_handler.h"

#include <optional>

#include "brave/browser/ui/brave_pages.h"
#include "build/build_config.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/notifications/notification_display_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
#include "url/gurl.h"

namespace brave_wallet {

BraveWalletNotificationHandler::BraveWalletNotificationHandler(Profile& profile)
    : profile_(profile) {}

BraveWalletNotificationHandler::~BraveWalletNotificationHandler() = default;

void BraveWalletNotificationHandler::OnClick(
    Profile* profile,
    const GURL& origin,
    const std::string& notification_id,
    const std::optional<int>& action_index,
    const std::optional<std::u16string>& reply,
    base::OnceClosure completed_closure) {
  BrowserWindowInterface* browser = nullptr;
  ForEachCurrentBrowserWindowInterfaceOrderedByActivation(
      [profile, &browser](BrowserWindowInterface* bwi) {
        if (bwi->GetProfile() == profile) {
          browser = bwi;
          return false;
        }
        return true;
      });

  if (browser) {
    brave::ShowBraveWalletTxNotificationUrl(browser, origin);
  }

  std::move(completed_closure).Run();
}

}  // namespace brave_wallet
