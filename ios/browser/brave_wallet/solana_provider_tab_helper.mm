// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_wallet/solana_provider_tab_helper.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/solana_provider_impl.h"
#include "brave/ios/browser/api/brave_wallet/brave_wallet_provider_delegate_ios+private.h"
#include "brave/ios/browser/brave_wallet/brave_wallet_service_factory.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "ios/chrome/browser/content_settings/model/host_content_settings_map_factory.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/js_messaging/web_frame.h"
#include "ios/web/public/js_messaging/web_frames_manager.h"
#include "ios/web/public/navigation/navigation_context.h"
#include "ios/web/public/web_state.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace brave_wallet {

namespace {

// Template for constructing a `solanaWeb3.PublicKey` from a base58 encoded key
// and emitting it via `window.solana.emit`. Expects `_braveSolanaWeb3` to
// already be injected by `SolanaProviderJavaScriptFeature`.
constexpr char kEmitPublicKeyEventScriptTemplate[] =
    "if (typeof _braveSolanaWeb3 !== 'undefined' && "
    "_braveSolanaWeb3.solanaWeb3) { window.solana.emit('%s', "
    "window.__gSafeBuiltins.deepFreeze(new "
    "_braveSolanaWeb3.solanaWeb3.PublicKey('%s'))) }";

// Template for assigning `window.solana.publicKey` to a frozen
// `solanaWeb3.PublicKey` constructed from a base58 encoded key.
constexpr char kSetPublicKeyPropertyScriptTemplate[] =
    "if (typeof _braveSolanaWeb3 !== 'undefined' && "
    "_braveSolanaWeb3.solanaWeb3) { window.solana.publicKey = "
    "window.__gSafeBuiltins.deepFreeze(new "
    "_braveSolanaWeb3.solanaWeb3.PublicKey('%s')) }";

void ExecuteJavaScript(web::WebState* web_state, const std::u16string& script) {
  if (!web_state) {
    return;
  }
  web::WebFramesManager* frames_manager =
      web_state->GetPageWorldWebFramesManager();
  if (!frames_manager) {
    return;
  }
  web::WebFrame* main_frame = frames_manager->GetMainWebFrame();
  if (!main_frame) {
    return;
  }
  main_frame->ExecuteJavaScript(script);
}

}  // namespace

SolanaProviderTabHelper::SolanaProviderTabHelper(
    web::WebState* web_state,
    BraveWalletService* brave_wallet_service,
    HostContentSettingsMap* host_content_settings_map)
    : web_state_(web_state),
      brave_wallet_service_(brave_wallet_service),
      host_content_settings_map_(host_content_settings_map) {
  CHECK(web_state_);
  web_state_->AddObserver(this);
}

SolanaProviderTabHelper::~SolanaProviderTabHelper() {
  if (web_state_) {
    web_state_->RemoveObserver(this);
  }
}

// static
void SolanaProviderTabHelper::MaybeCreateForWebState(web::WebState* web_state) {
  CHECK(web_state);
  ProfileIOS* profile =
      ProfileIOS::FromBrowserState(web_state->GetBrowserState());
  if (!brave_wallet::IsAllowed(profile->GetPrefs())) {
    return;
  }
  BraveWalletService* brave_wallet_service =
      BraveWalletServiceFactory::GetServiceForState(profile);
  HostContentSettingsMap* settings_map =
      ios::HostContentSettingsMapFactory::GetForProfile(profile);
  if (!brave_wallet_service || !settings_map) {
    return;
  }
  CreateForWebState(web_state, brave_wallet_service, settings_map);
}

void SolanaProviderTabHelper::SetBridge(
    id<BraveWalletProviderDelegate> bridge) {
  bridge_ = bridge;
}

mojom::SolanaProvider* SolanaProviderTabHelper::GetProvider() {
  return provider_.get();
}

void SolanaProviderTabHelper::UpdateSolanaProperties() {
  mojom::SolanaProvider* provider = GetProvider();
  if (!provider) {
    return;
  }
  provider->IsConnected(
      base::BindOnce(&SolanaProviderTabHelper::OnGetIsConnectedForProperties,
                     weak_ptr_factory_.GetWeakPtr()));
  provider->GetPublicKey(
      base::BindOnce(&SolanaProviderTabHelper::OnGetPublicKeyForProperties,
                     weak_ptr_factory_.GetWeakPtr()));
}

void SolanaProviderTabHelper::EmitConnectEvent(const std::string& public_key) {
  // `public_key` is base58 encoded so it is safe to embed directly.
  ExecuteJavaScript(web_state_, base::UTF8ToUTF16(absl::StrFormat(
                                    kEmitPublicKeyEventScriptTemplate,
                                    "connect", public_key)));
}

void SolanaProviderTabHelper::EmitDisconnectEvent() {
  ExecuteJavaScript(web_state_, u"window.solana.emit('disconnect')");
}

base::WeakPtr<SolanaProviderTabHelper> SolanaProviderTabHelper::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void SolanaProviderTabHelper::AccountChangedEvent(
    const std::optional<std::string>& account) {
  if (account) {
    // `account` is base58 encoded so it is safe to embed directly.
    ExecuteJavaScript(web_state_, base::UTF8ToUTF16(absl::StrFormat(
                                      kEmitPublicKeyEventScriptTemplate,
                                      "accountChanged", *account)));
  } else {
    ExecuteJavaScript(web_state_, u"window.solana.emit('accountChanged')");
  }
  UpdateSolanaProperties();
}

void SolanaProviderTabHelper::DisconnectEvent() {
  EmitDisconnectEvent();
}

void SolanaProviderTabHelper::OnGetIsConnectedForProperties(bool is_connected) {
  ExecuteJavaScript(
      web_state_,
      base::UTF8ToUTF16(base::StrCat(
          {"window.solana.isConnected = ", is_connected ? "true" : "false"})));
}

void SolanaProviderTabHelper::OnGetPublicKeyForProperties(
    const std::string& public_key) {
  if (public_key.empty()) {
    return;
  }
  // `public_key` is base58 encoded so it is safe to embed directly.
  ExecuteJavaScript(web_state_,
                    base::UTF8ToUTF16(absl::StrFormat(
                        kSetPublicKeyPropertyScriptTemplate, public_key)));
}

void SolanaProviderTabHelper::CreateProvider() {
  provider_.reset();
  events_receiver_.reset();

  if (!brave_wallet_service_ || !host_content_settings_map_) {
    return;
  }

  provider_ = std::make_unique<SolanaProviderImpl>(
      *host_content_settings_map_, brave_wallet_service_,
      std::make_unique<BraveWalletProviderDelegateBridge>(bridge_),
      url::Origin::Create(web_state_->GetLastCommittedURL()));

  // Register this tab helper as the provider's events listener so the provider
  // can emit `accountChanged` / `disconnect` events to the page.
  mojom::SolanaProvider* provider = provider_.get();
  provider->Init(events_receiver_.BindNewPipeAndPassRemote());
}

void SolanaProviderTabHelper::DidFinishNavigation(
    web::WebState* web_state,
    web::NavigationContext* navigation_context) {
  if (!navigation_context->HasCommitted()) {
    return;
  }

  // Providers must be recreated when the origin changes to align with desktop
  // (see `BraveContentBrowserClient::RegisterBrowserInterfaceBindersForFrame`).
  CreateProvider();
}

void SolanaProviderTabHelper::WebStateDestroyed(web::WebState* web_state) {
  provider_.reset();
  web_state_->RemoveObserver(this);
  web_state_ = nullptr;
}

}  // namespace brave_wallet
