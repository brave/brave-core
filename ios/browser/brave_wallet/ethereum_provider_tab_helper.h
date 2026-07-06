// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_WALLET_ETHEREUM_PROVIDER_TAB_HELPER_H_
#define BRAVE_IOS_BROWSER_BRAVE_WALLET_ETHEREUM_PROVIDER_TAB_HELPER_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "ios/web/public/web_state_observer.h"
#include "ios/web/public/web_state_user_data.h"
#include "mojo/public/cpp/bindings/receiver.h"

@protocol BraveWalletProviderDelegate;

namespace web {
class NavigationContext;
class WebState;
}  // namespace web

class HostContentSettingsMap;

namespace brave_wallet {

namespace mojom {
class EthereumProvider;
}  // namespace mojom

class BraveWalletService;
class EthereumProviderImpl;

// Owns the per-tab `EthereumProviderImpl` that backs the `window.ethereum`
// provider injected by `EthereumProviderJavaScriptFeature`. The provider is
// (re)created on every committed navigation so it is always scoped to the
// current origin to align with desktop's per-frame binding.
class EthereumProviderTabHelper
    : public web::WebStateUserData<EthereumProviderTabHelper>,
      public web::WebStateObserver,
      public mojom::EventsListener {
 public:
  EthereumProviderTabHelper(const EthereumProviderTabHelper&) = delete;
  EthereumProviderTabHelper& operator=(const EthereumProviderTabHelper&) =
      delete;
  ~EthereumProviderTabHelper() override;

  // Creates the tab helper for `web_state` if a `BraveWalletService` is
  // available for its profile.
  static void MaybeCreateForWebState(web::WebState* web_state);

  void SetBridge(id<BraveWalletProviderDelegate> bridge);

  // Returns the provider for the tab's current committed origin, or nullptr if
  // one could not be created (e.g. the wallet service is unavailable).
  mojom::EthereumProvider* GetProvider();

  // Refreshes the `chainId` / `networkVersion` / `selectedAddress` properties
  // on the page's `window.ethereum` object by evaluating JavaScript in the main
  // frame.
  void UpdateEthereumProperties();

  base::WeakPtr<EthereumProviderTabHelper> GetWeakPtr();

  // mojom::EventsListener
  void AccountsChangedEvent(const std::vector<std::string>& accounts) override;
  void ChainChangedEvent(const std::string& chain_id) override;
  void MessageEvent(const std::string& subscription_id,
                    base::Value result) override;

 private:
  friend class web::WebStateUserData<EthereumProviderTabHelper>;

  EthereumProviderTabHelper(web::WebState* web_state,
                            BraveWalletService* brave_wallet_service,
                            HostContentSettingsMap* host_content_settings_map);

  // Recreates `provider_` scoped to the web state's current committed origin.
  void CreateProvider();

  // Assigns the `window.ethereum` properties once the current chain id is
  // known.
  void OnGetChainIdForProperties(const std::string& chain_id);

  // Emits the `chainChanged` event and refreshes properties once the provider's
  // current chain id is known, but only if it matches the changed chain.
  void OnChainChangedGetChainId(const std::string& chain_id,
                                const std::string& provider_chain_id);

  // web::WebStateObserver:
  void DidFinishNavigation(web::WebState* web_state,
                           web::NavigationContext* navigation_context) override;
  void WebStateDestroyed(web::WebState* web_state) override;

  raw_ptr<web::WebState> web_state_;
  raw_ptr<BraveWalletService> brave_wallet_service_;
  raw_ptr<HostContentSettingsMap> host_content_settings_map_;
  std::unique_ptr<EthereumProviderImpl> provider_;
  __weak id<BraveWalletProviderDelegate> bridge_ = nil;
  mojo::Receiver<mojom::EventsListener> events_receiver_{this};

  base::WeakPtrFactory<EthereumProviderTabHelper> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_IOS_BROWSER_BRAVE_WALLET_ETHEREUM_PROVIDER_TAB_HELPER_H_
