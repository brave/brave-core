/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_wallet_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/web_ui.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

void RemoveEthereumChain(PrefService* prefs,
                         const std::string& chain_id_to_remove) {
  brave_wallet::RemoveCustomNetwork(prefs, chain_id_to_remove);
}

brave_wallet::mojom::NetworkInfoPtr GetEthereumChain(
    const std::string& payload,
    std::string* error_message) {
  CHECK(error_message);
  error_message->clear();
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          payload, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                       base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    *error_message = l10n_util::GetStringUTF8(
        IDS_SETTINGS_WALLET_NETWORKS_SUMBISSION_FAILED);
    return nullptr;
  }

  brave_wallet::mojom::NetworkInfoPtr chain =
      brave_wallet::ValueToEthNetworkInfo(records_v.value());
  if (!chain) {
    *error_message = l10n_util::GetStringUTF8(
        IDS_SETTINGS_WALLET_NETWORKS_SUMBISSION_FAILED);
    return nullptr;
  }
  return chain;
}

}  // namespace

BraveWalletHandler::BraveWalletHandler() {}
BraveWalletHandler::~BraveWalletHandler() {}

void BraveWalletHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getAutoLockMinutes",
      base::BindRepeating(&BraveWalletHandler::GetAutoLockMinutesDeprecated,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "removeEthereumChain",
      base::BindRepeating(&BraveWalletHandler::RemoveEthereumChainDeprecated,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getCustomNetworksList",
      base::BindRepeating(&BraveWalletHandler::GetCustomNetworksListDeprecated,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "addEthereumChain",
      base::BindRepeating(&BraveWalletHandler::AddEthereumChainDeprecated,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setActiveNetwork",
      base::BindRepeating(&BraveWalletHandler::SetActiveNetworkDeprecated,
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
  ::RemoveEthereumChain(prefs, args[1].GetString());
  ResolveJavascriptCallback(args[0], base::Value(true));
}

void BraveWalletHandler::GetCustomNetworksList(const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  PrefService* prefs = Profile::FromWebUI(web_ui())->GetPrefs();
  base::Value list(base::Value::Type::LIST);
  std::vector<brave_wallet::mojom::NetworkInfoPtr> custom_chains;
  brave_wallet::GetAllEthCustomChains(prefs, &custom_chains);
  for (const auto& it : custom_chains) {
    list.Append(brave_wallet::EthNetworkInfoToValue(it));
  }
  std::string json_string;
  base::JSONWriter::Write(list, &json_string);
  AllowJavascript();
  ResolveJavascriptCallback(args[0], base::Value(json_string));
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
  std::string error_message;
  auto* json_rpc_service =
      brave_wallet::JsonRpcServiceFactory::GetServiceForContext(
          Profile::FromWebUI(web_ui()));

  auto chain = GetEthereumChain(args[1].GetString(), &error_message);
  if (chain && json_rpc_service) {
    json_rpc_service->AddEthereumChain(
        chain->Clone(),
        base::BindOnce(&BraveWalletHandler::OnAddEthereumChain,
                       weak_ptr_factory_.GetWeakPtr(), args[0].Clone()));
    return;
  }
  auto message = error_message.empty()
                     ? l10n_util::GetStringUTF8(
                           IDS_SETTINGS_WALLET_NETWORKS_SUMBISSION_FAILED)
                     : error_message;

  base::ListValue result;
  result.Append(base::Value(false));
  result.Append(base::Value(message));
  ResolveJavascriptCallback(args[0], std::move(result));
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

// Remove this method and pass it the non-Deprecated version instead to
// RegisterMessageCallback once CL 3483819 lands (in Chromium 101).
void BraveWalletHandler::AddEthereumChainDeprecated(
    base::Value::ConstListView args) {
  base::Value::List args_list;
  for (const auto& arg : args)
    args_list.Append(arg.Clone());
  AddEthereumChain(args_list);
}

// Remove this method and pass it the non-Deprecated version instead to
// RegisterMessageCallback once CL 3483819 lands (in Chromium 101).
void BraveWalletHandler::SetActiveNetworkDeprecated(
    base::Value::ConstListView args) {
  base::Value::List args_list;
  for (const auto& arg : args)
    args_list.Append(arg.Clone());
  SetActiveNetwork(args_list);
}

// Remove this method and pass it the non-Deprecated version instead to
// RegisterMessageCallback once CL 3483819 lands (in Chromium 101).
void BraveWalletHandler::GetAutoLockMinutesDeprecated(
    base::Value::ConstListView args) {
  base::Value::List args_list;
  for (const auto& arg : args)
    args_list.Append(arg.Clone());
  GetAutoLockMinutes(args_list);
}

// Remove this method and pass it the non-Deprecated version instead to
// RegisterMessageCallback once CL 3483819 lands (in Chromium 101).
void BraveWalletHandler::RemoveEthereumChainDeprecated(
    base::Value::ConstListView args) {
  base::Value::List args_list;
  for (const auto& arg : args)
    args_list.Append(arg.Clone());
  RemoveEthereumChain(args_list);
}

// Remove this method and pass it the non-Deprecated version instead to
// RegisterMessageCallback once CL 3483819 lands (in Chromium 101).
void BraveWalletHandler::GetCustomNetworksListDeprecated(
    base::Value::ConstListView args) {
  base::Value::List args_list;
  for (const auto& arg : args)
    args_list.Append(arg.Clone());
  GetCustomNetworksList(args_list);
}
