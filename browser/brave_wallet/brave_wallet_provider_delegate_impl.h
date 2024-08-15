/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IMPL_H_

#include <optional>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/brave_wallet_provider_delegate.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-forward.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class Page;
class RenderFrameHost;
class WebContents;
}  // namespace content

namespace brave_wallet {

class BraveWalletProviderDelegateImpl : public BraveWalletProviderDelegate,
                                        public content::WebContentsObserver {
 public:
  explicit BraveWalletProviderDelegateImpl(
      content::WebContents* web_contents,
      content::RenderFrameHost* const render_frame_host);
  BraveWalletProviderDelegateImpl(const BraveWalletProviderDelegateImpl&) =
      delete;
  BraveWalletProviderDelegateImpl& operator=(
      const BraveWalletProviderDelegateImpl&) = delete;
  ~BraveWalletProviderDelegateImpl() override;

  bool IsTabVisible() override;
  void ShowPanel() override;
  void ShowWalletBackup() override;
  void UnlockWallet() override;
  void WalletInteractionDetected() override;
  void ShowWalletOnboarding() override;
  void ShowAccountCreation(mojom::CoinType type) override;
  url::Origin GetOrigin() const override;
  std::optional<std::vector<std::string>> GetAllowedAccounts(
      mojom::CoinType type,
      const std::vector<std::string>& accounts) override;
  void RequestPermissions(mojom::CoinType type,
                          const std::vector<std::string>& accounts,
                          RequestPermissionsCallback callback) override;
  bool IsAccountAllowed(mojom::CoinType type,
                        const std::string& account) override;
  bool IsPermissionDenied(mojom::CoinType type) override;
  void AddSolanaConnectedAccount(const std::string& account) override;
  void RemoveSolanaConnectedAccount(const std::string& account) override;
  bool IsSolanaAccountConnected(const std::string& account) override;

 private:
  // content::WebContentsObserver overrides
  void WebContentsDestroyed() override;
  void RenderFrameHostChanged(content::RenderFrameHost* old_host,
                              content::RenderFrameHost* new_host) override;
  void PrimaryPageChanged(content::Page& page) override;

  raw_ptr<content::WebContents> web_contents_ = nullptr;
  const content::GlobalRenderFrameHostId host_id_;
  base::WeakPtrFactory<BraveWalletProviderDelegateImpl> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IMPL_H_
