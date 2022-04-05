/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_SOLANA_PROVIDER_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_BRAVE_WALLET_SOLANA_PROVIDER_DELEGATE_IMPL_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/solana_provider_delegate.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace content {
class RenderFrameHost;
class WebContents;
}  // namespace content

namespace brave_wallet {

class KeyringService;

class SolanaProviderDelegateImpl : public SolanaProviderDelegate,
                                   public content::WebContentsObserver {
 public:
  explicit SolanaProviderDelegateImpl(
      content::WebContents* web_contents,
      content::RenderFrameHost* const render_frame_host);
  SolanaProviderDelegateImpl(const SolanaProviderDelegateImpl&) = delete;
  SolanaProviderDelegateImpl& operator=(const SolanaProviderDelegateImpl&) =
      delete;
  ~SolanaProviderDelegateImpl() override;

  void ShowPanel() override;
  GURL GetOrigin() const override;
  void RequestSolanaPermission(
      RequestSolanaPermissionCallback callback) override;
  void IsSelectedAccountAllowed(
      IsSelectedAccountAllowedCallback callback) override;

 private:
  void ContinueRequestSolanaPermission(
      RequestSolanaPermissionCallback callback,
      const absl::optional<std::string>& selected_account,
      bool is_selected_account_allowed);

  void ContinueIsSelectedAccountAllowed(
      IsSelectedAccountAllowedCallback callback,
      const absl::optional<std::string>& selected_account);

  // content::WebContentsObserver overrides
  void WebContentsDestroyed() override;

  mojo::Remote<mojom::KeyringService> keyring_service_;
  raw_ptr<content::WebContents> web_contents_ = nullptr;
  const content::GlobalRenderFrameHostId host_id_;
  base::WeakPtrFactory<SolanaProviderDelegateImpl> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_SOLANA_PROVIDER_DELEGATE_IMPL_H_
