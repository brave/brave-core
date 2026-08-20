// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_wallet/cardano_provider_tab_helper.h"

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_provider_impl.h"
#include "brave/ios/browser/api/brave_wallet/brave_wallet_provider_delegate_ios+private.h"
#include "brave/ios/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/ios/browser/brave_wallet/brave_wallet_utils.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/navigation/navigation_context.h"
#include "ios/web/public/web_state.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "url/origin.h"

namespace brave_wallet {

CardanoProviderTabHelper::CardanoProviderTabHelper(
    web::WebState* web_state,
    BraveWalletService* brave_wallet_service)
    : web_state_(web_state), brave_wallet_service_(brave_wallet_service) {
  CHECK(web_state_);
  web_state_->AddObserver(this);
}

CardanoProviderTabHelper::~CardanoProviderTabHelper() {
  if (web_state_) {
    web_state_->RemoveObserver(this);
  }
}

// static
void CardanoProviderTabHelper::MaybeCreateForWebState(
    web::WebState* web_state) {
  CHECK(web_state);
  ProfileIOS* profile =
      ProfileIOS::FromBrowserState(web_state->GetBrowserState());
  if (!brave_wallet::IsAllowed(profile->GetPrefs())) {
    return;
  }
  BraveWalletService* brave_wallet_service =
      BraveWalletServiceFactory::GetServiceForState(profile);
  if (!brave_wallet_service) {
    return;
  }
  CreateForWebState(web_state, brave_wallet_service);
}

void CardanoProviderTabHelper::SetBridge(
    id<BraveWalletProviderDelegate> bridge) {
  bridge_ = bridge;
}

mojom::CardanoProvider* CardanoProviderTabHelper::GetProvider() {
  return provider_.get();
}

void CardanoProviderTabHelper::BindCardanoApi(
    mojo::PendingRemote<mojom::CardanoApi> api) {
  cardano_api_.reset();
  if (api) {
    cardano_api_.Bind(std::move(api));
  }
}

mojom::CardanoApi* CardanoProviderTabHelper::GetCardanoApi() {
  return cardano_api_.is_bound() ? cardano_api_.get() : nullptr;
}

base::WeakPtr<CardanoProviderTabHelper> CardanoProviderTabHelper::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void CardanoProviderTabHelper::CreateProvider() {
  provider_.reset();
  cardano_api_.reset();

  PrefService* prefs =
      ProfileIOS::FromBrowserState(web_state_->GetBrowserState())->GetPrefs();

  if (!brave_wallet_service_ || !IsDefaultCardanoWalletBrave(prefs)) {
    return;
  }

  provider_ = std::make_unique<CardanoProviderImpl>(
      *brave_wallet_service_,
      base::BindRepeating(
          [](id<BraveWalletProviderDelegate> bridge)
              -> std::unique_ptr<BraveWalletProviderDelegate> {
            return std::make_unique<BraveWalletProviderDelegateBridge>(bridge);
          },
          bridge_),
      url::Origin::Create(web_state_->GetLastCommittedURL()));
}

void CardanoProviderTabHelper::DidFinishNavigation(
    web::WebState* web_state,
    web::NavigationContext* navigation_context) {
  if (!navigation_context->HasCommitted() ||
      navigation_context->IsSameDocument()) {
    return;
  }

  // Providers must be recreated when the origin changes to align with desktop
  // (see `BraveContentBrowserClient::RegisterBrowserInterfaceBindersForFrame`).
  CreateProvider();
}

void CardanoProviderTabHelper::WebStateDestroyed(web::WebState* web_state) {
  cardano_api_.reset();
  provider_.reset();
  web_state_->RemoveObserver(this);
  web_state_ = nullptr;
}

}  // namespace brave_wallet
