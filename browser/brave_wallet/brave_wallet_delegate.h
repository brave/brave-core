/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_DELEGATE_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_DELEGATE_H_

#include "brave/components/brave_wallet/browser/browser_wallet_delegate.h"

class BraveWalletDelegate : public BrowserWalletDelegate {
 public:
  ~BraveWalletDelegate() override;
  void CloseTabsAndRestart() override;
};

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_DELEGATE_H_
