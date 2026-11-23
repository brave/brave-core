/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PERMISSIONS_BRAVE_WALLET_ACCOUNT_PERMISSION_CONTEXT_FACTORY_H_
#define BRAVE_BROWSER_PERMISSIONS_BRAVE_WALLET_ACCOUNT_PERMISSION_CONTEXT_FACTORY_H_

#include <memory>

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace content {
class BrowserContext;
}

namespace permissions {
class BraveWalletAccountPermissionContext;
}

class BraveWalletAccountPermissionContextFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static permissions::BraveWalletAccountPermissionContext* GetForProfile(
      content::BrowserContext* profile);
  static BraveWalletAccountPermissionContextFactory* GetInstance();

 private:
  friend base::NoDestructor<BraveWalletAccountPermissionContextFactory>;

  BraveWalletAccountPermissionContextFactory();
  BraveWalletAccountPermissionContextFactory(
      const BraveWalletAccountPermissionContextFactory&) = delete;
  BraveWalletAccountPermissionContextFactory& operator=(
      const BraveWalletAccountPermissionContextFactory&) = delete;
  ~BraveWalletAccountPermissionContextFactory() override;

  // BrowserContextKeyedServiceFactory methods:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

#endif  // BRAVE_BROWSER_PERMISSIONS_BRAVE_WALLET_ACCOUNT_PERMISSION_CONTEXT_FACTORY_H_
