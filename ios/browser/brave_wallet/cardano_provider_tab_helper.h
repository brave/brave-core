// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_WALLET_CARDANO_PROVIDER_TAB_HELPER_H_
#define BRAVE_IOS_BROWSER_BRAVE_WALLET_CARDANO_PROVIDER_TAB_HELPER_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "ios/web/public/web_state_observer.h"
#include "ios/web/public/web_state_user_data.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"

@protocol BraveWalletProviderDelegate;

namespace web {
class NavigationContext;
class WebState;
}  // namespace web

namespace brave_wallet {

namespace mojom {
class CardanoApi;
class CardanoProvider;
}  // namespace mojom

class BraveWalletService;
class CardanoProviderImpl;

// Owns the per-tab `CardanoProviderImpl` that backs the `window.cardano.brave`
// provider injected by `CardanoProviderJavaScriptFeature`. The provider is
// (re)created on every committed navigation so it is always scoped to the
// current origin to align with desktop's per-frame binding. Once the page
// calls `enable()`, the resulting `CardanoApi` remote is retained here and used
// to service the CIP-30 method calls posted from the page.
class CardanoProviderTabHelper
    : public web::WebStateUserData<CardanoProviderTabHelper>,
      public web::WebStateObserver {
 public:
  CardanoProviderTabHelper(const CardanoProviderTabHelper&) = delete;
  CardanoProviderTabHelper& operator=(const CardanoProviderTabHelper&) = delete;
  ~CardanoProviderTabHelper() override;

  // Creates the tab helper for `web_state` if a `BraveWalletService` is
  // available for its profile.
  static void MaybeCreateForWebState(web::WebState* web_state);

  void SetBridge(id<BraveWalletProviderDelegate> bridge);

  // Returns the provider for the tab's current committed origin, or nullptr if
  // one could not be created (e.g. the wallet service is unavailable).
  mojom::CardanoProvider* GetProvider();

  // Binds the `CardanoApi` remote returned by a successful `enable()` so
  // subsequent CIP-30 method calls can be serviced. Any previously bound remote
  // is replaced.
  void BindCardanoApi(mojo::PendingRemote<mojom::CardanoApi> api);

  // Returns the `CardanoApi` remote bound after a successful `enable()`, or
  // nullptr if the page has not enabled the provider.
  mojom::CardanoApi* GetCardanoApi();

  base::WeakPtr<CardanoProviderTabHelper> GetWeakPtr();

 private:
  friend class web::WebStateUserData<CardanoProviderTabHelper>;

  CardanoProviderTabHelper(web::WebState* web_state,
                           BraveWalletService* brave_wallet_service);

  // Recreates `provider_` scoped to the web state's current committed origin
  // and resets any bound `CardanoApi` remote.
  void CreateProvider();

  // web::WebStateObserver:
  void DidFinishNavigation(web::WebState* web_state,
                           web::NavigationContext* navigation_context) override;
  void WebStateDestroyed(web::WebState* web_state) override;

  raw_ptr<web::WebState> web_state_;
  raw_ptr<BraveWalletService> brave_wallet_service_;
  std::unique_ptr<CardanoProviderImpl> provider_;
  mojo::Remote<mojom::CardanoApi> cardano_api_;
  __weak id<BraveWalletProviderDelegate> bridge_ = nil;

  base::WeakPtrFactory<CardanoProviderTabHelper> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_IOS_BROWSER_BRAVE_WALLET_CARDANO_PROVIDER_TAB_HELPER_H_
