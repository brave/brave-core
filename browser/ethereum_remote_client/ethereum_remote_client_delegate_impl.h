/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_ETHEREUM_REMOTE_CLIENT_ETHEREUM_REMOTE_CLIENT_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_ETHEREUM_REMOTE_CLIENT_ETHEREUM_REMOTE_CLIENT_DELEGATE_IMPL_H_

#include "brave/browser/ethereum_remote_client/ethereum_remote_client_delegate.h"

class EthereumRemoteClientDelegateImpl : public EthereumRemoteClientDelegate {
 public:
  ~EthereumRemoteClientDelegateImpl() override;
  void MaybeLoadCryptoWalletsExtension(
      content::BrowserContext* context) override;
};

#endif  // BRAVE_BROWSER_ETHEREUM_REMOTE_CLIENT_ETHEREUM_REMOTE_CLIENT_DELEGATE_IMPL_H_
