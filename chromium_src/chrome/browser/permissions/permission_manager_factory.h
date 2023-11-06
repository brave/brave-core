/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PERMISSIONS_PERMISSION_MANAGER_FACTORY_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PERMISSIONS_PERMISSION_MANAGER_FACTORY_H_

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace brave_wallet {
class EthereumProviderImplUnitTest;
class SolanaProviderImplUnitTest;
class BraveWalletServiceUnitTest;
}  // namespace brave_wallet

namespace permissions {
class BraveWalletPermissionContextUnitTest;
}

#define BuildServiceInstanceForBrowserContext               \
  BuildServiceInstanceForBrowserContext_ChromiumImpl(       \
      content::BrowserContext* profile) const;              \
  friend brave_wallet::EthereumProviderImplUnitTest;        \
  friend brave_wallet::SolanaProviderImplUnitTest;          \
  friend brave_wallet::BraveWalletServiceUnitTest;          \
  friend permissions::BraveWalletPermissionContextUnitTest; \
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext

#include "src/chrome/browser/permissions/permission_manager_factory.h"  // IWYU pragma: export
#undef BuildServiceInstanceForBrowserContext

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PERMISSIONS_PERMISSION_MANAGER_FACTORY_H_
