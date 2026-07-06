// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_wallet/ethereum_provider_tab_helper.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/ethereum_provider_impl.h"
#include "brave/components/brave_wallet/common/web_ui_constants.h"
#include "brave/ios/browser/api/brave_wallet/brave_wallet_provider_delegate_ios+private.h"
#include "brave/ios/browser/brave_wallet/brave_wallet_service_factory.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "ios/chrome/browser/content_settings/model/host_content_settings_map_factory.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/components/webui/web_ui_url_constants.h"
#include "ios/web/public/js_messaging/content_world.h"
#include "ios/web/public/js_messaging/web_frame.h"
#include "ios/web/public/js_messaging/web_frames_manager.h"
#include "ios/web/public/navigation/navigation_context.h"
#include "ios/web/public/navigation/navigation_manager.h"
#include "ios/web/public/web_state.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace brave_wallet {

namespace {

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

// Dispatches an event to the page via `window.ethereum.emit(name, arguments)`.
void EmitEthereumEvent(web::WebState* web_state,
                       std::string_view event_name,
                       std::optional<base::Value> arguments) {
  std::string name_json;
  base::JSONWriter::Write(base::Value(event_name), &name_json);

  std::string script;
  if (arguments) {
    std::string arguments_json;
    base::JSONWriter::Write(*arguments, &arguments_json);
    script = base::StrCat(
        {"window.ethereum.emit(", name_json, ", ", arguments_json, ")"});
  } else {
    script = base::StrCat({"window.ethereum.emit(", name_json, ")"});
  }
  ExecuteJavaScript(web_state, base::UTF8ToUTF16(script));
}

}  // namespace

EthereumProviderTabHelper::EthereumProviderTabHelper(
    web::WebState* web_state,
    BraveWalletService* brave_wallet_service,
    HostContentSettingsMap* host_content_settings_map)
    : web_state_(web_state),
      brave_wallet_service_(brave_wallet_service),
      host_content_settings_map_(host_content_settings_map) {
  CHECK(web_state_);
  web_state_->AddObserver(this);
}

EthereumProviderTabHelper::~EthereumProviderTabHelper() {
  if (web_state_) {
    web_state_->RemoveObserver(this);
  }
}

// static
void EthereumProviderTabHelper::MaybeCreateForWebState(
    web::WebState* web_state) {
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

void EthereumProviderTabHelper::SetBridge(
    id<BraveWalletProviderDelegate> bridge) {
  bridge_ = bridge;
}

mojom::EthereumProvider* EthereumProviderTabHelper::GetProvider() {
  return provider_.get();
}

void EthereumProviderTabHelper::UpdateEthereumProperties() {
  mojom::EthereumProvider* provider = GetProvider();
  if (!provider) {
    return;
  }
  provider->GetChainId(
      base::BindOnce(&EthereumProviderTabHelper::OnGetChainIdForProperties,
                     weak_ptr_factory_.GetWeakPtr()));
}

base::WeakPtr<EthereumProviderTabHelper>
EthereumProviderTabHelper::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void EthereumProviderTabHelper::AccountsChangedEvent(
    const std::vector<std::string>& accounts) {
  // Only the first allowed account is exposed to the page
  base::ListValue event_accounts;
  if (!accounts.empty()) {
    event_accounts.Append(accounts.front());
  }
  EmitEthereumEvent(web_state_, "accountsChanged",
                    base::Value(std::move(event_accounts)));

  UpdateEthereumProperties();
}

void EthereumProviderTabHelper::ChainChangedEvent(const std::string& chain_id) {
  mojom::EthereumProvider* provider = GetProvider();
  if (!provider) {
    return;
  }
  // The chain change might not apply to this origin when a new default network
  // is assigned so only emit once we've confirmed the provider's current chain
  // id matches the changed chain.
  provider->GetChainId(
      base::BindOnce(&EthereumProviderTabHelper::OnChainChangedGetChainId,
                     weak_ptr_factory_.GetWeakPtr(), chain_id));
}

void EthereumProviderTabHelper::OnChainChangedGetChainId(
    const std::string& chain_id,
    const std::string& provider_chain_id) {
  if (provider_chain_id != chain_id) {
    return;
  }

  EmitEthereumEvent(web_state_, "chainChanged", base::Value(chain_id));
  UpdateEthereumProperties();

  // Temporary fix for #5404: the dapp is not updated on chain change without a
  // reload, so - like MetaMask - reload the tab on chain changes.
  const GURL visible_url = web_state_->GetVisibleURL();
  const bool is_wallet_webui_page = visible_url.scheme() == kChromeUIScheme &&
                                    visible_url.host() == kWalletPageHost;
  if (web_state_ && !is_wallet_webui_page) {
    web_state_->GetNavigationManager()->Reload(web::ReloadType::NORMAL,
                                               /*check_for_repost=*/false);
  }
}

void EthereumProviderTabHelper::MessageEvent(const std::string& subscription_id,
                                             base::Value result) {
  base::DictValue data;
  data.Set("subscription", subscription_id);
  data.Set("result", std::move(result));

  base::DictValue arguments;
  arguments.Set("type", "eth_subscription");
  arguments.Set("data", std::move(data));

  EmitEthereumEvent(web_state_, "message", base::Value(std::move(arguments)));
}

void EthereumProviderTabHelper::OnGetChainIdForProperties(
    const std::string& chain_id) {
  if (!provider_) {
    return;
  }

  // `chainId` is always assigned as a quoted hex string, e.g.
  // `window.ethereum.chainId = "0x1"`.
  ExecuteJavaScript(web_state_,
                    base::UTF8ToUTF16(base::StrCat(
                        {"window.ethereum.chainId = \"", chain_id, "\""})));

  // `networkVersion` is the decimal form of the chain id, or the literal string
  // "undefined" when it cannot be parsed.
  std::string network_version = "undefined";
  std::string_view hex = chain_id;
  if (base::StartsWith(hex, "0x", base::CompareCase::INSENSITIVE_ASCII)) {
    hex.remove_prefix(2);
  }
  if (uint32_t parsed = 0; base::HexStringToUInt(hex, &parsed)) {
    network_version = base::NumberToString(parsed);
  }
  ExecuteJavaScript(
      web_state_,
      base::UTF8ToUTF16(base::StrCat(
          {"window.ethereum.networkVersion = \"", network_version, "\""})));

  // `selectedAddress` is the first allowed account (quoted), or bare
  // `undefined` when the keyring is locked / there are no allowed accounts.
  // `GetAllowedAccounts(false)` already returns nullopt when the keyring is
  // locked.
  std::string selected_address = "undefined";
  std::optional<std::vector<std::string>> allowed_accounts =
      provider_->GetAllowedAccounts(/*include_accounts_when_locked=*/false);
  if (allowed_accounts && !allowed_accounts->empty()) {
    selected_address = base::StrCat({"\"", allowed_accounts->front(), "\""});
  }
  ExecuteJavaScript(
      web_state_,
      base::UTF8ToUTF16(base::StrCat(
          {"window.ethereum.selectedAddress = ", selected_address})));
}

void EthereumProviderTabHelper::CreateProvider() {
  provider_.reset();
  events_receiver_.reset();

  if (!brave_wallet_service_ || !host_content_settings_map_) {
    return;
  }

  ProfileIOS* profile =
      ProfileIOS::FromBrowserState(web_state_->GetBrowserState());

  provider_ = std::make_unique<EthereumProviderImpl>(
      host_content_settings_map_, brave_wallet_service_,
      std::make_unique<BraveWalletProviderDelegateBridge>(bridge_),
      profile->GetPrefs(),
      url::Origin::Create(web_state_->GetLastCommittedURL()));

  // Register this tab helper as the provider's events listener so the provider
  // can emit `chainChanged` / `accountsChanged` / `message` events to the page.
  mojom::EthereumProvider* provider = provider_.get();
  provider->Init(events_receiver_.BindNewPipeAndPassRemote());
}

void EthereumProviderTabHelper::DidFinishNavigation(
    web::WebState* web_state,
    web::NavigationContext* navigation_context) {
  if (!navigation_context->HasCommitted()) {
    return;
  }

  // Providers must be recreated when the origin changes to align with desktop
  // (see `BraveContentBrowserClient::RegisterBrowserInterfaceBindersForFrame`).
  CreateProvider();
}

void EthereumProviderTabHelper::WebStateDestroyed(web::WebState* web_state) {
  provider_.reset();
  web_state_->RemoveObserver(this);
  web_state_ = nullptr;
}

}  // namespace brave_wallet
