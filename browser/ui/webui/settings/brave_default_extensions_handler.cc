/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_default_extensions_handler.h"

#include <memory>
#include <string>

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/extensions/brave_component_loader.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_webtorrent/grit/brave_webtorrent_resources.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/decentralized_dns/core/constants.h"
#include "brave/components/decentralized_dns/core/utils.h"
#include "brave/components/l10n/common/localization_util.h"
#include "chrome/browser/about_flags.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/component_loader.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/webstore_install_with_prompt.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/media/router/media_router_feature.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/webui/flags/flags_ui_constants.h"
#include "components/webui/flags/pref_service_flags_storage.h"
#include "content/public/browser/web_ui.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/constants.h"
#include "extensions/common/feature_switch.h"
#include "ui/shell_dialogs/selected_file_info.h"

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_constants.h"
#endif

#if BUILDFLAG(ENABLE_WIDEVINE)
#include "brave/browser/widevine/widevine_utils.h"
#endif

using decentralized_dns::EnsOffchainResolveMethod;
using decentralized_dns::ResolveMethodTypes;

namespace {

template <typename T>
base::Value::Dict MakeSelectValue(T value, const std::u16string& name) {
  base::Value::Dict item;
  item.Set("value", base::Value(static_cast<int>(value)));
  item.Set("name", base::Value(name));
  return item;
}

base::Value::List GetResolveMethodList() {
  base::Value::List list;
  list.Append(MakeSelectValue(ResolveMethodTypes::ASK,
                              brave_l10n::GetLocalizedResourceUTF16String(
                                  IDS_DECENTRALIZED_DNS_RESOLVE_OPTION_ASK)));
  list.Append(
      MakeSelectValue(ResolveMethodTypes::DISABLED,
                      brave_l10n::GetLocalizedResourceUTF16String(
                          IDS_DECENTRALIZED_DNS_RESOLVE_OPTION_DISABLED)));
  list.Append(
      MakeSelectValue(ResolveMethodTypes::ENABLED,
                      brave_l10n::GetLocalizedResourceUTF16String(
                          IDS_DECENTRALIZED_DNS_RESOLVE_OPTION_ENABLED)));

  return list;
}

base::Value::List GetEnsOffchainResolveMethodList() {
  base::Value::List list;
  list.Append(MakeSelectValue(
      EnsOffchainResolveMethod::kAsk,
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_DECENTRALIZED_DNS_ENS_OFFCHAIN_RESOLVE_OPTION_ASK)));
  list.Append(MakeSelectValue(
      EnsOffchainResolveMethod::kDisabled,
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_DECENTRALIZED_DNS_ENS_OFFCHAIN_RESOLVE_OPTION_DISABLED)));
  list.Append(MakeSelectValue(
      EnsOffchainResolveMethod::kEnabled,
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_DECENTRALIZED_DNS_ENS_OFFCHAIN_RESOLVE_OPTION_ENABLED)));

  return list;
}
}  // namespace

BraveDefaultExtensionsHandler::BraveDefaultExtensionsHandler()
    : weak_ptr_factory_(this) {
#if BUILDFLAG(ENABLE_WIDEVINE)
  was_widevine_enabled_ = ::IsWidevineEnabled();
#endif
}

BraveDefaultExtensionsHandler::~BraveDefaultExtensionsHandler() = default;

void BraveDefaultExtensionsHandler::RegisterMessages() {
  profile_ = Profile::FromWebUI(web_ui());
  web_ui()->RegisterMessageCallback(
      "resetWallet",
      base::BindRepeating(&BraveDefaultExtensionsHandler::ResetWallet,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "resetTransactionInfo",
      base::BindRepeating(&BraveDefaultExtensionsHandler::ResetTransactionInfo,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "setWebTorrentEnabled",
      base::BindRepeating(&BraveDefaultExtensionsHandler::SetWebTorrentEnabled,
                          base::Unretained(this)));
#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
  web_ui()->RegisterMessageCallback(
      "setBraveWalletEnabled",
      base::BindRepeating(&BraveDefaultExtensionsHandler::SetBraveWalletEnabled,
                          base::Unretained(this)));
#endif

  // TODO(petemill): If anything outside this handler is responsible for causing
  // restart-neccessary actions, then this should be moved to a generic handler
  // and the flag should be moved to somewhere more static / singleton-like.
  web_ui()->RegisterMessageCallback(
      "getRestartNeeded",
      base::BindRepeating(&BraveDefaultExtensionsHandler::GetRestartNeeded,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setWidevineEnabled",
      base::BindRepeating(&BraveDefaultExtensionsHandler::SetWidevineEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "isWidevineEnabled",
      base::BindRepeating(&BraveDefaultExtensionsHandler::IsWidevineEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getDecentralizedDnsResolveMethodList",
      base::BindRepeating(
          &BraveDefaultExtensionsHandler::GetDecentralizedDnsResolveMethodList,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getEnsOffchainResolveMethodList",
      base::BindRepeating(
          &BraveDefaultExtensionsHandler::GetEnsOffchainResolveMethodList,
          base::Unretained(this)));

  // Can't call this in ctor because it needs to access web_ui().
  InitializePrefCallbacks();
}

void BraveDefaultExtensionsHandler::InitializePrefCallbacks() {
#if BUILDFLAG(ENABLE_WIDEVINE)
  local_state_change_registrar_.Init(g_browser_process->local_state());
  local_state_change_registrar_.Add(
      kWidevineEnabled,
      base::BindRepeating(
          &BraveDefaultExtensionsHandler::OnWidevineEnabledChanged,
          base::Unretained(this)));
#endif
  pref_change_registrar_.Init(profile_->GetPrefs());
  pref_change_registrar_.Add(
      kDefaultEthereumWallet,
      base::BindRepeating(&BraveDefaultExtensionsHandler::OnWalletTypeChanged,
                          base::Unretained(this)));
}

bool BraveDefaultExtensionsHandler::IsRestartNeeded() {
#if BUILDFLAG(ENABLE_WIDEVINE)
  if (was_widevine_enabled_ != ::IsWidevineEnabled()) {
    return true;
  }
#endif

  return false;
}

void BraveDefaultExtensionsHandler::GetRestartNeeded(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);

  AllowJavascript();
  ResolveJavascriptCallback(args[0], base::Value(IsRestartNeeded()));
}

void BraveDefaultExtensionsHandler::ResetWallet(const base::Value::List& args) {
  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForContext(profile_);
  if (brave_wallet_service) {
    brave_wallet_service->Reset();
  }
}

void BraveDefaultExtensionsHandler::ResetTransactionInfo(
    const base::Value::List& args) {
  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForContext(profile_);
  if (brave_wallet_service) {
    brave_wallet_service->tx_service()->Reset();
  }
}

void BraveDefaultExtensionsHandler::SetWebTorrentEnabled(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);
  bool enabled = args[0].GetBool();

  extensions::ExtensionService* service =
      extensions::ExtensionSystem::Get(profile_)->extension_service();
  extensions::ComponentLoader* loader =
      extensions::ComponentLoader::Get(profile_);

  if (enabled) {
    if (!loader->Exists(brave_webtorrent_extension_id)) {
      base::FilePath brave_webtorrent_path(FILE_PATH_LITERAL(""));
      brave_webtorrent_path =
          brave_webtorrent_path.Append(FILE_PATH_LITERAL("brave_webtorrent"));
      loader->Add(IDR_BRAVE_WEBTORRENT, brave_webtorrent_path);
    }
    service->EnableExtension(brave_webtorrent_extension_id);
  } else {
    service->DisableExtension(
        brave_webtorrent_extension_id,
        extensions::disable_reason::DisableReason::DISABLE_BLOCKED_BY_POLICY);
  }
}

bool BraveDefaultExtensionsHandler::IsExtensionInstalled(
    const std::string& extension_id) const {
  extensions::ExtensionRegistry* registry = extensions::ExtensionRegistry::Get(
      static_cast<content::BrowserContext*>(profile_));
  return registry && registry->GetInstalledExtension(extension_id);
}

void BraveDefaultExtensionsHandler::OnInstallResult(
    const std::string& pref_name,
    bool success,
    const std::string& error,
    extensions::webstore_install::Result result) {
  if (result != extensions::webstore_install::Result::SUCCESS &&
      result != extensions::webstore_install::Result::LAUNCH_IN_PROGRESS) {
    profile_->GetPrefs()->SetBoolean(pref_name, false);
  }
}

void BraveDefaultExtensionsHandler::OnRestartNeededChanged() {
  if (IsJavascriptAllowed()) {
    FireWebUIListener("brave-needs-restart-changed",
                      base::Value(IsRestartNeeded()));
  }
}

void BraveDefaultExtensionsHandler::SetWidevineEnabled(
    const base::Value::List& args) {
#if BUILDFLAG(ENABLE_WIDEVINE)
  CHECK_EQ(args.size(), 1U);
  bool enabled = args[0].GetBool();
  enabled ? EnableWidevineCdm() : DisableWidevineCdm();
  AllowJavascript();
#endif
}

void BraveDefaultExtensionsHandler::IsWidevineEnabled(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  AllowJavascript();
  ResolveJavascriptCallback(args[0],
#if BUILDFLAG(ENABLE_WIDEVINE)
                            base::Value(::IsWidevineEnabled()));
#else
                            base::Value(false));
#endif
}

void BraveDefaultExtensionsHandler::OnWalletTypeChanged() {
  if (brave_wallet::GetDefaultEthereumWallet(profile_->GetPrefs()) ==
      brave_wallet::mojom::DefaultWallet::CryptoWallets) {
    return;
  }
  extensions::ExtensionService* service =
      extensions::ExtensionSystem::Get(profile_)->extension_service();
  service->DisableExtension(
      kEthereumRemoteClientExtensionId,
      extensions::disable_reason::DisableReason::DISABLE_USER_ACTION);
}

void BraveDefaultExtensionsHandler::OnWidevineEnabledChanged() {
  if (IsJavascriptAllowed()) {
    FireWebUIListener("widevine-enabled-changed",
#if BUILDFLAG(ENABLE_WIDEVINE)
                      base::Value(::IsWidevineEnabled()));
#else
                      base::Value(false));
#endif
    OnRestartNeededChanged();
  }
}

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
void BraveDefaultExtensionsHandler::SetBraveWalletEnabled(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);
  bool enabled = args[0].GetBool();

  extensions::ExtensionService* service =
      extensions::ExtensionSystem::Get(profile_)->extension_service();
  if (enabled) {
    service->EnableExtension(kEthereumRemoteClientExtensionId);
  } else {
    service->DisableExtension(
        kEthereumRemoteClientExtensionId,
        extensions::disable_reason::DisableReason::DISABLE_USER_ACTION);
  }
}
#endif

void BraveDefaultExtensionsHandler::GetDecentralizedDnsResolveMethodList(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  AllowJavascript();

  ResolveJavascriptCallback(args[0], ::GetResolveMethodList());
}

void BraveDefaultExtensionsHandler::GetEnsOffchainResolveMethodList(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  AllowJavascript();

  ResolveJavascriptCallback(args[0],
                            base::Value(::GetEnsOffchainResolveMethodList()));
}
