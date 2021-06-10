/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IMPL_H_

#include "brave/components/brave_wallet/browser/brave_wallet_provider_delegate.h"

namespace content {
class WebContents;
}  // namespace content

namespace brave_wallet {

class BraveWalletProviderDelegateImpl : public BraveWalletProviderDelegate {
 public:
  explicit BraveWalletProviderDelegateImpl(content::WebContents* web_contents);
  BraveWalletProviderDelegateImpl(const BraveWalletProviderDelegateImpl&) =
      delete;
  BraveWalletProviderDelegateImpl& operator=(
      const BraveWalletProviderDelegateImpl&) = delete;
  ~BraveWalletProviderDelegateImpl() override = default;

  void ShowConnectToSiteUI() override;

 private:
  content::WebContents* web_contents_;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IMPL_H_
