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
}  // namespace brave_wallet

namespace permissions {
class BraveWalletPermissionContextUnitTest;
}

#define BuildServiceInstanceFor                                          \
  BuildServiceInstanceFor_ChromiumImpl(content::BrowserContext* profile) \
      const;                                                             \
  friend brave_wallet::EthereumProviderImplUnitTest;                     \
  friend brave_wallet::SolanaProviderImplUnitTest;                       \
  friend permissions::BraveWalletPermissionContextUnitTest;              \
  KeyedService* BuildServiceInstanceFor

#include "src/chrome/browser/permissions/permission_manager_factory.h"
#undef BuildServiceInstanceFor

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PERMISSIONS_PERMISSION_MANAGER_FACTORY_H_
