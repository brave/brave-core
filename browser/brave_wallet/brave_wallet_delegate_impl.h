/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_DELEGATE_IMPL_H_

#include "brave/components/brave_wallet/brave_wallet_delegate.h"

class BraveWalletDelegateImpl : public BraveWalletDelegate {
 public:
  ~BraveWalletDelegateImpl() override;
  void MaybeLoadCryptoWalletsExtension(
      content::BrowserContext* context) override;
};

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_DELEGATE_IMPL_H_
