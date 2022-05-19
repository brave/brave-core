/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_WALLET_NOTIFICATION_HELPER_H_
#define BRAVE_BROWSER_BRAVE_WALLET_WALLET_NOTIFICATION_HELPER_H_

namespace content {
class BrowserContext;
}  // namespace content

namespace brave_wallet {

class TxService;

void RegisterWalletNotificationService(content::BrowserContext* context,
                                       TxService* service);

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_WALLET_NOTIFICATION_HELPER_H_
