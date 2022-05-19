/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/wallet_notification_helper.h"

#include "brave/browser/brave_wallet/notifications/wallet_notification_service_factory.h"
#include "brave/components/brave_wallet/browser/tx_service.h"

namespace brave_wallet {

void RegisterWalletNotificationService(content::BrowserContext* context,
                                       TxService* tx_service) {
  auto* notification_service =
      WalletNotificationServiceFactory::GetInstance()->GetServiceForContext(
          context);
  tx_service->AddObserver(notification_service->GetReceiver());
}

}  // namespace brave_wallet
