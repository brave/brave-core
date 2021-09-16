/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IMPL_ANDROID_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IMPL_ANDROID_H_

#include <string>

#include "brave/components/brave_wallet/browser/brave_wallet_provider_delegate.h"

namespace content {
class RenderFrameHost;
class WebContents;
}  // namespace content

namespace brave_wallet {

class BraveWalletProviderDelegateImplAndroid
    : public BraveWalletProviderDelegate {
 public:
  explicit BraveWalletProviderDelegateImplAndroid(
      content::WebContents* web_contents,
      content::RenderFrameHost* const render_frame_host);
  BraveWalletProviderDelegateImplAndroid(
      const BraveWalletProviderDelegateImplAndroid&) = delete;
  BraveWalletProviderDelegateImplAndroid& operator=(
      const BraveWalletProviderDelegateImplAndroid&) = delete;
  ~BraveWalletProviderDelegateImplAndroid() override = default;

  void RequestEthereumPermissions(
      RequestEthereumPermissionsCallback callback) override;
  void GetAllowedAccounts(GetAllowedAccountsCallback callback) override;
  GURL GetOrigin() const override;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IMPL_ANDROID_H_
