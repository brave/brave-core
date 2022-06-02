/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_wallet_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/feature_list.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/web_ui.h"
#include "ui/base/l10n/l10n_util.h"

BraveWalletHandler::BraveWalletHandler() = default;
BraveWalletHandler::~BraveWalletHandler() = default;

void BraveWalletHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getAutoLockMinutes",
      base::BindRepeating(&BraveWalletHandler::GetAutoLockMinutes,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "removeEthereumChain",
      base::BindRepeating(&BraveWalletHandler::RemoveEthereumChain,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getCustomNetworksList",
      base::BindRepeating(&BraveWalletHandler::GetCustomNetworksList,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "addEthereumChain",
      base::BindRepeating(&BraveWalletHandler::AddEthereumChain,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setActiveNetwork",
      base::BindRepeating(&BraveWalletHandler::SetActiveNetwork,
                          base::Unretained(this)));
}

void BraveWalletHandler::GetAutoLockMinutes(const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  PrefService* prefs = Profile::FromWebUI(web_ui())->GetPrefs();
  AllowJavascript();
  ResolveJavascriptCallback(
      args[0], base::Value(prefs->GetInteger(kBraveWalletAutoLockMinutes)));
}

void BraveWalletHandler::RemoveEthereumChain(const base::Value::List& args) {
  CHECK_EQ(args.size(), 2U);
  PrefService* prefs = Profile::FromWebUI(web_ui())->GetPrefs();
  AllowJavascript();
  brave_wallet::RemoveCustomNetwork(prefs, args[1].GetString());
  ResolveJavascriptCallback(args[0], base::Value(true));
}

void BraveWalletHandler::GetCustomNetworksList(const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  PrefService* prefs = Profile::FromWebUI(web_ui())->GetPrefs();
  base::Value list(base::Value::Type::LIST);

  if (base::FeatureList::IsEnabled(
          brave_wallet::features::kBraveWalletEditKnownNetworksFeature)) {
    for (const auto& it : brave_wallet::GetAllChains(
             prefs, brave_wallet::mojom::CoinType::ETH)) {
      list.Append(brave_wallet::EthNetworkInfoToValue(*it));
    }
  } else {
    for (const auto& it : brave_wallet::GetAllEthCustomChains(prefs)) {
      list.Append(brave_wallet::EthNetworkInfoToValue(*it));
    }
  }
  AllowJavascript();
  ResolveJavascriptCallback(args[0], std::move(list));
}

void BraveWalletHandler::OnAddEthereumChain(
    base::Value javascript_callback,
    const std::string& chain_id,
    brave_wallet::mojom::ProviderError error,
    const std::string& error_message) {
  base::ListValue result;
  result.Append(
      base::Value(error == brave_wallet::mojom::ProviderError::kSuccess));
  result.Append(base::Value(error_message));
  ResolveJavascriptCallback(javascript_callback, std::move(result));
  if (chain_callback_for_testing_)
    std::move(chain_callback_for_testing_).Run();
}

void BraveWalletHandler::AddEthereumChain(const base::Value::List& args) {
  CHECK_EQ(args.size(), 2U);
  AllowJavascript();
  auto* json_rpc_service =
      brave_wallet::JsonRpcServiceFactory::GetServiceForContext(
          Profile::FromWebUI(web_ui()));

  brave_wallet::mojom::NetworkInfoPtr chain =
      brave_wallet::ValueToEthNetworkInfo(args[1]);

  if (!chain || !json_rpc_service) {
    base::ListValue result;
    result.Append(base::Value(false));
    result.Append(base::Value(l10n_util::GetStringUTF8(
        IDS_SETTINGS_WALLET_NETWORKS_SUMBISSION_FAILED)));
    ResolveJavascriptCallback(args[0], std::move(result));
    return;
  }

  json_rpc_service->AddEthereumChain(
      std::move(chain),
      base::BindOnce(&BraveWalletHandler::OnAddEthereumChain,
                     weak_ptr_factory_.GetWeakPtr(), args[0].Clone()));
}

void BraveWalletHandler::SetActiveNetwork(const base::Value::List& args) {
  CHECK_EQ(args.size(), 2U);
  AllowJavascript();
  auto* json_rpc_service =
      brave_wallet::JsonRpcServiceFactory::GetServiceForContext(
          Profile::FromWebUI(web_ui()));
  auto result = json_rpc_service ? json_rpc_service->SetNetwork(
                                       args[1].GetString(),
                                       brave_wallet::mojom::CoinType::ETH)
                                 : false;
  ResolveJavascriptCallback(args[0], base::Value(result));
}
