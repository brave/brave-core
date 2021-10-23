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
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/web_ui.h"

#include "brave/components/brave_wallet/browser/pref_names.h"

namespace {

bool RemoveEthereumChain(PrefService* prefs,
                         const std::string& chain_id_to_remove) {
  std::vector<::brave_wallet::mojom::EthereumChainPtr> custom_chains;
  ::brave_wallet::GetAllCustomChains(prefs, &custom_chains);

  {
    ListPrefUpdate update(prefs, kBraveWalletCustomNetworks);
    base::ListValue* list = update.Get();
    list->EraseListValueIf([&](const base::Value& v) {
      auto* chain_id_value = v.FindStringKey("chainId");
      if (!chain_id_value)
        return false;
      return *chain_id_value == chain_id_to_remove;
    });
  }
  return true;
}

bool AddEthereumChain(PrefService* prefs, const std::string& payload) {
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          payload, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    return false;
  }

  const base::DictionaryValue* params_dict = nullptr;
  if (!records_v->GetAsDictionary(&params_dict) || !params_dict) {
    return false;
  }

  const std::string* chain_id = params_dict->FindStringKey("chainId");
  if (!chain_id) {
    return false;
  }
  std::vector<::brave_wallet::mojom::EthereumChainPtr> custom_chains;
  ::brave_wallet::GetAllChains(prefs, &custom_chains);
  bool network_exists = false;
  for (const auto& it : custom_chains) {
    if (it->chain_id != *chain_id)
      continue;
    network_exists = true;
    break;
  }
  if (network_exists) {
    return false;
  }
  const auto& value_to_add = records_v.value();
  auto chain = brave_wallet::ValueToEthereumChain(value_to_add);
  if (!chain) {
    return false;
  }

  // Saving converted value to initialized missed by user field with default
  // values
  auto value =
      brave_wallet::EthereumChainToValue(std::move(chain).value().Clone());
  {
    ListPrefUpdate update(prefs, kBraveWalletCustomNetworks);
    base::ListValue* list = update.Get();
    list->Append(std::move(value));
  }
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
  ResolveJavascriptCallback(
      args[0], base::Value(::RemoveEthereumChain(prefs, args[1].GetString())));
}

void BraveWalletHandler::GetCustomNetworksList(
    base::Value::ConstListView args) {
  CHECK_EQ(args.size(), 1U);
  PrefService* prefs = Profile::FromWebUI(web_ui())->GetPrefs();
  base::Value list(base::Value::Type::LIST);
  std::vector<brave_wallet::mojom::EthereumChainPtr> custom_chains;
  brave_wallet::GetAllCustomChains(prefs, &custom_chains);
  for (const auto& it : custom_chains) {
    list.Append(::brave_wallet::EthereumChainToValue(it));
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
  ResolveJavascriptCallback(
      args[0], base::Value(::AddEthereumChain(prefs, args[1].GetString())));
}
