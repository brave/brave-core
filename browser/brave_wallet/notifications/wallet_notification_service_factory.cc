/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/notifications/wallet_notification_service_factory.h"

#include <memory>

#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/brave_wallet/notifications/wallet_notification_service.h"
#include "brave/browser/brave_wallet/tx_service_factory.h"
#include "chrome/browser/notifications/notification_display_service_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace brave_wallet {

// static
WalletNotificationServiceFactory*
WalletNotificationServiceFactory::GetInstance() {
  return base::Singleton<WalletNotificationServiceFactory>::get();
}

WalletNotificationServiceFactory::WalletNotificationServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "WalletNotificationService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(NotificationDisplayServiceFactory::GetInstance());
  DependsOn(brave_wallet::TxServiceFactory::GetInstance());
}

WalletNotificationServiceFactory::~WalletNotificationServiceFactory() = default;

KeyedService* WalletNotificationServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new WalletNotificationService(context);
}

// static
WalletNotificationService*
WalletNotificationServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  return static_cast<WalletNotificationService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

}  // namespace brave_wallet
