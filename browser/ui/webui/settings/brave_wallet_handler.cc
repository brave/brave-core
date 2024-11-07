/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_wallet_handler.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/notreached.h"
#include "base/values.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

base::Value::Dict MakeSelectValue(const std::u16string& name,
                                  ::brave_wallet::mojom::DefaultWallet value) {
  base::Value::Dict item;
  item.Set("value", static_cast<int>(value));
  item.Set("name", name);
  return item;
}

base::Value::Dict MakeSelectValue(
    const std::u16string& name,
    ::brave_wallet::mojom::BlowfishOptInStatus value) {
  base::Value::Dict item;
  item.Set("value", static_cast<int>(value));
  item.Set("name", name);
  return item;
}

std::optional<brave_wallet::mojom::CoinType> ToCoinType(
    std::optional<int> val) {
  if (!val) {
    return std::nullopt;
  }
  auto result = static_cast<brave_wallet::mojom::CoinType>(*val);
  if (result != brave_wallet::mojom::CoinType::ETH &&
      result != brave_wallet::mojom::CoinType::FIL &&
      result != brave_wallet::mojom::CoinType::SOL &&
      result != brave_wallet::mojom::CoinType::BTC &&
      result != brave_wallet::mojom::CoinType::ZEC) {
    return std::nullopt;
  }
  return result;
}

}  // namespace

BraveWalletHandler::BraveWalletHandler() = default;
BraveWalletHandler::~BraveWalletHandler() = default;

void BraveWalletHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getAutoLockMinutes",
      base::BindRepeating(&BraveWalletHandler::GetAutoLockMinutes,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getSolanaProviderOptions",
      base::BindRepeating(&BraveWalletHandler::GetSolanaProviderOptions,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getTransactionSimulationOptInStatusOptions",
      base::BindRepeating(
          &BraveWalletHandler::GetTransactionSimulationOptInStatusOptions,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "removeChain", base::BindRepeating(&BraveWalletHandler::RemoveChain,
                                         base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "resetChain", base::BindRepeating(&BraveWalletHandler::ResetChain,
                                        base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getNetworksList",
      base::BindRepeating(&BraveWalletHandler::GetNetworksList,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getPrepopulatedNetworksList",
      base::BindRepeating(&BraveWalletHandler::GetPrepopulatedNetworksList,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "addChain", base::BindRepeating(&BraveWalletHandler::AddChain,
                                      base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setDefaultNetwork",
      base::BindRepeating(&BraveWalletHandler::SetDefaultNetwork,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "addHiddenNetwork",
      base::BindRepeating(&BraveWalletHandler::AddHiddenNetwork,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "removeHiddenNetwork",
      base::BindRepeating(&BraveWalletHandler::RemoveHiddenNetwork,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "isBitcoinEnabled",
      base::BindRepeating(&BraveWalletHandler::IsBitcoinEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "isZCashEnabled", base::BindRepeating(&BraveWalletHandler::IsZCashEnabled,
                                            base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "isTransactionSimulationsFeatureEnabled",
      base::BindRepeating(&BraveWalletHandler::IsTransactionSimulationsEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setWalletInPrivateWindowsEnabled",
      base::BindRepeating(&BraveWalletHandler::SetWalletInPrivateWindowsEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getWalletInPrivateWindowsEnabled",
      base::BindRepeating(&BraveWalletHandler::GetWalletInPrivateWindowsEnabled,
                          base::Unretained(this)));
}

void BraveWalletHandler::GetAutoLockMinutes(const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  AllowJavascript();
  ResolveJavascriptCallback(
      args[0],
      base::Value(GetPrefs()->GetInteger(kBraveWalletAutoLockMinutes)));
}

void BraveWalletHandler::GetSolanaProviderOptions(
    const base::Value::List& args) {
  base::Value::List list;
  list.Append(MakeSelectValue(
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_BRAVE_WALLET_WEB3_PROVIDER_BRAVE_PREFER_EXTENSIONS),
      ::brave_wallet::mojom::DefaultWallet::BraveWalletPreferExtension));
  list.Append(
      MakeSelectValue(brave_l10n::GetLocalizedResourceUTF16String(
                          IDS_BRAVE_WALLET_WEB3_PROVIDER_BRAVE),
                      ::brave_wallet::mojom::DefaultWallet::BraveWallet));
  list.Append(MakeSelectValue(brave_l10n::GetLocalizedResourceUTF16String(
                                  IDS_BRAVE_WALLET_WEB3_PROVIDER_NONE),
                              ::brave_wallet::mojom::DefaultWallet::None));
  CHECK_EQ(args.size(), 1U);
  AllowJavascript();
  ResolveJavascriptCallback(args[0], list);
}

void BraveWalletHandler::GetTransactionSimulationOptInStatusOptions(
    const base::Value::List& args) {
  base::Value::List list;
  list.Append(
      MakeSelectValue(brave_l10n::GetLocalizedResourceUTF16String(
                          IDS_SETTINGS_SELECT_VALUE_ASK),
                      ::brave_wallet::mojom::BlowfishOptInStatus::kUnset));
  list.Append(
      MakeSelectValue(brave_l10n::GetLocalizedResourceUTF16String(
                          IDS_SETTINGS_SELECT_VALUE_YES),
                      ::brave_wallet::mojom::BlowfishOptInStatus::kAllowed));
  list.Append(MakeSelectValue(
      brave_l10n::GetLocalizedResourceUTF16String(IDS_SETTINGS_SELECT_VALUE_NO),
      ::brave_wallet::mojom::BlowfishOptInStatus::kDenied));

  CHECK_EQ(args.size(), 1U);
  AllowJavascript();
  ResolveJavascriptCallback(args[0], list);
}

void BraveWalletHandler::RemoveChain(const base::Value::List& args) {
  CHECK_EQ(args.size(), 3U);
  AllowJavascript();

  auto* chain_id = args[1].GetIfString();
  auto coin = ToCoinType(args[2].GetIfInt());
  if (!chain_id || !coin) {
    ResolveJavascriptCallback(args[0], base::Value());
    return;
  }

  GetNetworkManager()->RemoveCustomNetwork(*chain_id, *coin);
  ResolveJavascriptCallback(args[0], base::Value(true));
}

void BraveWalletHandler::ResetChain(const base::Value::List& args) {
  CHECK_EQ(args.size(), 3U);
  AllowJavascript();

  auto* chain_id = args[1].GetIfString();
  auto coin = ToCoinType(args[2].GetIfInt());
  if (!chain_id || !coin) {
    ResolveJavascriptCallback(args[0], base::Value());
    return;
  }

  DCHECK(GetNetworkManager()->CustomChainExists(*chain_id, *coin));
  GetNetworkManager()->RemoveCustomNetwork(*chain_id, *coin);
  DCHECK(GetNetworkManager()->KnownChainExists(*chain_id, *coin));
  ResolveJavascriptCallback(args[0], base::Value(true));
}

void BraveWalletHandler::GetNetworksList(const base::Value::List& args) {
  CHECK_EQ(args.size(), 2U);
  base::Value::Dict result;
  auto coin = ToCoinType(args[1].GetIfInt());
  if (!coin) {
    ResolveJavascriptCallback(args[0], base::Value());
    return;
  }

  result.Set("defaultNetwork",
             GetNetworkManager()->GetCurrentChainId(*coin, std::nullopt));

  auto& networks = result.Set("networks", base::Value::List())->GetList();
  for (const auto& it : GetNetworkManager()->GetAllChains()) {
    if (it->coin == coin) {
      networks.Append(brave_wallet::NetworkInfoToValue(*it));
    }
  }
  auto& known_networks =
      result.Set("knownNetworks", base::Value::List())->GetList();
  for (const auto& it : GetNetworkManager()->GetAllKnownChains(*coin)) {
    known_networks.Append(it->chain_id);
  }

  auto& custom_networks =
      result.Set("customNetworks", base::Value::List())->GetList();
  for (const auto& it : GetNetworkManager()->GetAllCustomChains(*coin)) {
    custom_networks.Append(it->chain_id);
  }

  auto& hidden_networks =
      result.Set("hiddenNetworks", base::Value::List())->GetList();
  for (const auto& it : GetNetworkManager()->GetHiddenNetworks(*coin)) {
    hidden_networks.Append(it);
  }

  AllowJavascript();
  ResolveJavascriptCallback(args[0], result);
}

void BraveWalletHandler::GetPrepopulatedNetworksList(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  AllowJavascript();

  base::Value::List networks;

  auto* blockchain_registry = brave_wallet::BlockchainRegistry::GetInstance();
  if (!blockchain_registry) {
    ResolveJavascriptCallback(args[0], networks);
    return;
  }

  for (const auto& it : blockchain_registry->GetPrepopulatedNetworks()) {
    networks.Append(brave_wallet::NetworkInfoToValue(*it));
  }

  ResolveJavascriptCallback(args[0], networks);
}

void BraveWalletHandler::OnAddChain(base::Value javascript_callback,
                                    const std::string& chain_id,
                                    brave_wallet::mojom::ProviderError error,
                                    const std::string& error_message) {
  base::Value::List result;
  result.Append(error == brave_wallet::mojom::ProviderError::kSuccess);
  result.Append(error_message);
  ResolveJavascriptCallback(javascript_callback, result);
  if (chain_callback_for_testing_) {
    std::move(chain_callback_for_testing_).Run();
  }
}

void BraveWalletHandler::AddChain(const base::Value::List& args) {
  CHECK_EQ(args.size(), 2U);
  AllowJavascript();
  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
          Profile::FromWebUI(web_ui()));

  brave_wallet::mojom::NetworkInfoPtr chain =
      brave_wallet::ValueToNetworkInfo(args[1]);

  if (!chain || !brave_wallet_service) {
    base::Value::List result;
    result.Append(false);
    result.Append(l10n_util::GetStringUTF8(
        IDS_SETTINGS_WALLET_NETWORKS_SUMBISSION_FAILED));
    ResolveJavascriptCallback(args[0], result);
    return;
  }

  brave_wallet_service->json_rpc_service()->AddChain(
      std::move(chain),
      base::BindOnce(&BraveWalletHandler::OnAddChain,
                     weak_ptr_factory_.GetWeakPtr(), args[0].Clone()));
}

void BraveWalletHandler::SetDefaultNetwork(const base::Value::List& args) {
  CHECK_EQ(args.size(), 3U);

  auto* chain_id = args[1].GetIfString();
  auto coin = ToCoinType(args[2].GetIfInt());
  if (!chain_id || !coin) {
    ResolveJavascriptCallback(args[0], base::Value());
    return;
  }

  AllowJavascript();
  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
          Profile::FromWebUI(web_ui()));
  auto result = brave_wallet_service
                    ? brave_wallet_service->json_rpc_service()->SetNetwork(
                          *chain_id, *coin, std::nullopt)
                    : false;
  ResolveJavascriptCallback(args[0], base::Value(result));
}

void BraveWalletHandler::AddHiddenNetwork(const base::Value::List& args) {
  CHECK_EQ(args.size(), 3U);
  auto* chain_id = args[1].GetIfString();
  auto coin = ToCoinType(args[2].GetIfInt());
  if (!chain_id || !coin) {
    ResolveJavascriptCallback(args[0], base::Value());
    return;
  }

  AllowJavascript();
  GetNetworkManager()->AddHiddenNetwork(*coin, *chain_id);
  ResolveJavascriptCallback(args[0], base::Value(true));
}

void BraveWalletHandler::RemoveHiddenNetwork(const base::Value::List& args) {
  CHECK_EQ(args.size(), 3U);
  auto* chain_id = args[1].GetIfString();
  auto coin = ToCoinType(args[2].GetIfInt());
  if (!chain_id || !coin) {
    ResolveJavascriptCallback(args[0], base::Value());
    return;
  }

  AllowJavascript();
  GetNetworkManager()->RemoveHiddenNetwork(*coin, *chain_id);
  ResolveJavascriptCallback(args[0], base::Value(true));
}

PrefService* BraveWalletHandler::GetPrefs() {
  return Profile::FromWebUI(web_ui())->GetPrefs();
}

brave_wallet::NetworkManager* BraveWalletHandler::GetNetworkManager() {
  return brave_wallet::BraveWalletServiceFactory::GetInstance()
      ->GetServiceForContext(Profile::FromWebUI(web_ui()))
      ->network_manager();
}

void BraveWalletHandler::IsBitcoinEnabled(const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  AllowJavascript();
  ResolveJavascriptCallback(args[0],
                            base::Value(::brave_wallet::IsBitcoinEnabled()));
}

void BraveWalletHandler::IsZCashEnabled(const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  AllowJavascript();
  ResolveJavascriptCallback(args[0],
                            base::Value(::brave_wallet::IsZCashEnabled()));
}

void BraveWalletHandler::IsTransactionSimulationsEnabled(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  AllowJavascript();
  ResolveJavascriptCallback(
      args[0], base::Value(::brave_wallet::IsTransactionSimulationsEnabled()));
}

void BraveWalletHandler::SetWalletInPrivateWindowsEnabled(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 2U);
  bool enabled = args[1].GetBool();
  Profile::FromWebUI(web_ui())->GetPrefs()->SetBoolean(
      kBraveWalletPrivateWindowsEnabled, enabled);
  AllowJavascript();
  ResolveJavascriptCallback(args[0], base::Value(true));
}

void BraveWalletHandler::GetWalletInPrivateWindowsEnabled(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  bool enabled = Profile::FromWebUI(web_ui())->GetPrefs()->GetBoolean(
      kBraveWalletPrivateWindowsEnabled);
  AllowJavascript();
  ResolveJavascriptCallback(args[0], enabled);
}
