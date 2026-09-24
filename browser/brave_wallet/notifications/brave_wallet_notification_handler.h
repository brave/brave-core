/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_NOTIFICATIONS_BRAVE_WALLET_NOTIFICATION_HANDLER_H_
#define BRAVE_BROWSER_BRAVE_WALLET_NOTIFICATIONS_BRAVE_WALLET_NOTIFICATION_HANDLER_H_

#include <optional>
#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ref.h"
#include "chrome/browser/notifications/notification_handler.h"

class GURL;
class Profile;

namespace brave_wallet {

class BraveWalletNotificationHandler : public NotificationHandler {
 public:
  explicit BraveWalletNotificationHandler(Profile& profile);

  BraveWalletNotificationHandler(const BraveWalletNotificationHandler&) =
      delete;
  BraveWalletNotificationHandler& operator=(
      const BraveWalletNotificationHandler&) = delete;
  BraveWalletNotificationHandler(BraveWalletNotificationHandler&&) = delete;
  BraveWalletNotificationHandler& operator=(BraveWalletNotificationHandler&&) =
      delete;

  ~BraveWalletNotificationHandler() override;

  // NotificationHandler:
  void OnClick(Profile* profile,
               const GURL& origin,
               const std::string& notification_id,
               const std::optional<int>& action_index,
               const std::optional<std::u16string>& reply,
               base::OnceClosure completed_closure) override;

 private:
  const raw_ref<Profile> profile_;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_NOTIFICATIONS_BRAVE_WALLET_NOTIFICATION_HANDLER_H_
