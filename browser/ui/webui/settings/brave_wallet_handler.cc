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
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
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
  ListPrefUpdate update(prefs, kBraveWalletCustomNetworks);
  base::ListValue* list = update.Get();
  list->EraseListValueIf([&](const base::Value& v) {
    auto* chain_id_value = v.FindStringKey("chainId");
    if (!chain_id_value)
      return false;
    return *chain_id_value == chain_id_to_remove;
  });
}

bool AddEthereumChain(PrefService* prefs,
                      const std::string& payload,
                      std::string* error_message) {
  CHECK(error_message);
  error_message->clear();
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          payload, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    *error_message = l10n_util::GetStringUTF8(
        IDS_SETTINGS_WALLET_NETWORKS_SUMBISSION_FAILED);
    return false;
  }

  const base::DictionaryValue* params_dict = nullptr;
  if (!records_v->GetAsDictionary(&params_dict) || !params_dict) {
    *error_message = l10n_util::GetStringUTF8(
        IDS_SETTINGS_WALLET_NETWORKS_SUMBISSION_FAILED);
    return false;
  }

  const auto& value_to_add = records_v.value();

  // Saving converted value to initialize missing field with default values.
  absl::optional<brave_wallet::mojom::EthereumChain> chain =
      brave_wallet::ValueToEthereumChain(value_to_add);
  if (!chain) {
    *error_message = l10n_util::GetStringUTF8(
        IDS_SETTINGS_WALLET_NETWORKS_SUMBISSION_FAILED);
    return false;
  }

  std::vector<::brave_wallet::mojom::EthereumChainPtr> custom_chains;
  brave_wallet::GetAllChains(prefs, &custom_chains);
  for (const auto& it : custom_chains) {
    if (it->chain_id == chain->chain_id) {
      *error_message =
          l10n_util::GetStringUTF8(IDS_SETTINGS_WALLET_NETWORKS_EXISTS);
      return false;
    }
  }

  brave_wallet::AddCustomNetwork(prefs, chain.value().Clone());
  return true;
}

}  // namespace

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
}

void BraveWalletHandler::GetAutoLockMinutes(base::Value::ConstListView args) {
  CHECK_EQ(args.size(), 1U);
  PrefService* prefs = Profile::FromWebUI(web_ui())->GetPrefs();
  AllowJavascript();
  ResolveJavascriptCallback(
      args[0], base::Value(prefs->GetInteger(kBraveWalletAutoLockMinutes)));
}

void BraveWalletHandler::RemoveEthereumChain(base::Value::ConstListView args) {
  CHECK_EQ(args.size(), 2U);
  PrefService* prefs = Profile::FromWebUI(web_ui())->GetPrefs();
  AllowJavascript();
  ::RemoveEthereumChain(prefs, args[1].GetString());
  ResolveJavascriptCallback(args[0], base::Value(true));
}

void BraveWalletHandler::GetCustomNetworksList(
    base::Value::ConstListView args) {
  CHECK_EQ(args.size(), 1U);
  PrefService* prefs = Profile::FromWebUI(web_ui())->GetPrefs();
  base::Value list(base::Value::Type::LIST);
  std::vector<brave_wallet::mojom::EthereumChainPtr> custom_chains;
  brave_wallet::GetAllCustomChains(prefs, &custom_chains);
  for (const auto& it : custom_chains) {
    list.Append(brave_wallet::EthereumChainToValue(it));
  }
  std::string json_string;
  base::JSONWriter::Write(list, &json_string);
  AllowJavascript();
  ResolveJavascriptCallback(args[0], base::Value(json_string));
}

void BraveWalletHandler::AddEthereumChain(base::Value::ConstListView args) {
  CHECK_EQ(args.size(), 2U);
  PrefService* prefs = Profile::FromWebUI(web_ui())->GetPrefs();
  AllowJavascript();
  std::string error_message;
  bool success = ::AddEthereumChain(prefs, args[1].GetString(), &error_message);
  base::ListValue result;
  result.Append(base::Value(success));
  result.Append(base::Value(error_message));
  ResolveJavascriptCallback(args[0], std::move(result));
}
