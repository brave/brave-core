/* Copyright (c) 2021 The Brave Authors. All rights reserved.
+ * This Source Code Form is subject to the terms of the Mozilla Public
+ * License, v. 2.0. If a copy of the MPL was not distributed with this file,
+ * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ethereum_remote_client/ethereum_remote_client_delegate_impl.h"
#include "brave/browser/extensions/brave_component_loader.h"
#include "chrome/browser/extensions/extension_service.h"
#include "content/public/browser/browser_context.h"
#include "extensions/browser/extension_system.h"

EthereumRemoteClientDelegateImpl::~EthereumRemoteClientDelegateImpl() = default;

void EthereumRemoteClientDelegateImpl::MaybeLoadCryptoWalletsExtension(
    content::BrowserContext* context) {
  auto* loader = extensions::ComponentLoader::Get(context);
  if (loader) {
    static_cast<extensions::BraveComponentLoader*>(loader)
        ->AddEthereumRemoteClientExtension();
  }
}

void EthereumRemoteClientDelegateImpl::UnloadCryptoWalletsExtension(
    content::BrowserContext* context) {
  auto* loader = extensions::ComponentLoader::Get(context);
  if (loader) {
    static_cast<extensions::BraveComponentLoader*>(loader)
        ->UnloadEthereumRemoteClientExtension();
  }
}
