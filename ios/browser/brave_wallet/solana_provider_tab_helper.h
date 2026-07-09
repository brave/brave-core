// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_WALLET_SOLANA_PROVIDER_TAB_HELPER_H_
#define BRAVE_IOS_BROWSER_BRAVE_WALLET_SOLANA_PROVIDER_TAB_HELPER_H_

#include <memory>
#include <optional>
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
class SolanaProvider;
}  // namespace mojom

class BraveWalletService;
class SolanaProviderImpl;

// Owns the per-tab `SolanaProviderImpl` that backs the `window.solana` /
// `window.braveSolana` provider injected by `SolanaProviderJavaScriptFeature`.
// The provider is recreated on every committed navigation so it is always
// scoped to the current origin to align with desktop's per-frame binding.
class SolanaProviderTabHelper
    : public web::WebStateUserData<SolanaProviderTabHelper>,
      public web::WebStateObserver,
      public mojom::SolanaEventsListener {
 public:
  SolanaProviderTabHelper(const SolanaProviderTabHelper&) = delete;
  SolanaProviderTabHelper& operator=(const SolanaProviderTabHelper&) = delete;
  ~SolanaProviderTabHelper() override;

  // Creates the tab helper for `web_state` if a the required dependencies are
  // available
  static void MaybeCreateForWebState(web::WebState* web_state);

  void SetBridge(id<BraveWalletProviderDelegate> bridge);

  // Returns the provider for the tab's current committed origin, or nullptr if
  // one could not be created (e.g. the wallet service is unavailable).
  mojom::SolanaProvider* GetProvider();

  // Refreshes the `isConnected` / `publicKey` properties on the page's
  // `window.solana` object by evaluating JavaScript in the main frame.
  void UpdateSolanaProperties();

  // Emits the `connect` event to the page with the connected `public_key`
  // (base58 encoded) wrapped in a `solanaWeb3.PublicKey`.
  void EmitConnectEvent(const std::string& public_key);

  // Emits the `disconnect` event to the page.
  void EmitDisconnectEvent();

  base::WeakPtr<SolanaProviderTabHelper> GetWeakPtr();

  // mojom::SolanaEventsListener
  void AccountChangedEvent(const std::optional<std::string>& account) override;
  void DisconnectEvent() override;

 private:
  friend class web::WebStateUserData<SolanaProviderTabHelper>;

  SolanaProviderTabHelper(web::WebState* web_state,
                          BraveWalletService* brave_wallet_service,
                          HostContentSettingsMap* host_content_settings_map);

  // Recreates `provider_` scoped to the web state's current committed origin.
  void CreateProvider();

  // Assigns the `window.solana.isConnected` / `window.solana.publicKey`
  // properties once the provider reports its current state.
  void OnGetIsConnectedForProperties(bool is_connected);
  void OnGetPublicKeyForProperties(const std::string& public_key);

  // web::WebStateObserver:
  void DidFinishNavigation(web::WebState* web_state,
                           web::NavigationContext* navigation_context) override;
  void WebStateDestroyed(web::WebState* web_state) override;

  raw_ptr<web::WebState> web_state_;
  raw_ptr<BraveWalletService> brave_wallet_service_;
  raw_ptr<HostContentSettingsMap> host_content_settings_map_;
  std::unique_ptr<SolanaProviderImpl> provider_;
  __weak id<BraveWalletProviderDelegate> bridge_ = nil;
  mojo::Receiver<mojom::SolanaEventsListener> events_receiver_{this};

  base::WeakPtrFactory<SolanaProviderTabHelper> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_IOS_BROWSER_BRAVE_WALLET_SOLANA_PROVIDER_TAB_HELPER_H_
