/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_WALLET_CORE_BROWSER_NETWORK_WALLET_HTTP_CLIENT_IMPL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_WALLET_CORE_BROWSER_NETWORK_WALLET_HTTP_CLIENT_IMPL_H_

// We don't want to send wallet requests as Brave doesn't support Google wallet.
// Seems easier to replace upstream's class completely.

#include "base/memory/scoped_refptr.h"
#include "components/wallet/core/browser/network/wallet_http_client.h"

namespace signin {
class IdentityManager;
}

namespace network {
class SharedURLLoaderFactory;
}

namespace wallet {

class WalletHttpClientImpl : public WalletHttpClient {
 public:
  WalletHttpClientImpl(
      signin::IdentityManager* identity_manager,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~WalletHttpClientImpl() override;

  WalletHttpClientImpl(const WalletHttpClientImpl&) = delete;
  WalletHttpClientImpl& operator=(const WalletHttpClientImpl&) = delete;

  // WalletHttpClient:
  void SavePass(const WalletablePass& pass, SavePassCallback callback) override;
};

}  // namespace wallet

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_WALLET_CORE_BROWSER_NETWORK_WALLET_HTTP_CLIENT_IMPL_H_
