/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/wallet_util.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_keys.h"

namespace ledger {
namespace wallet {

namespace {

std::string WalletTypeToState(const std::string& wallet_type) {
  if (wallet_type == constant::kWalletBitflyer) {
    return state::kWalletBitflyer;
  } else if (wallet_type == constant::kWalletGemini) {
    return state::kWalletGemini;
  } else if (wallet_type == constant::kWalletUphold) {
    return state::kWalletUphold;
  }

  NOTREACHED();
  return "";
}

}  // namespace

type::ExternalWalletPtr ExternalWalletPtrFromJSON(std::string wallet_string,
                                                  std::string wallet_type) {
  absl::optional<base::Value> value = base::JSONReader::Read(wallet_string);
  if (!value || !value->is_dict()) {
    BLOG(0, "Parsing of " + wallet_type + " wallet failed");
    return nullptr;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Parsing of " + wallet_type + " wallet failed");
    return nullptr;
  }

  auto wallet = ledger::type::ExternalWallet::New();
  wallet->type = wallet_type;

  auto* token = dictionary->FindStringKey("token");
  if (token) {
    wallet->token = *token;
  }

  auto* address = dictionary->FindStringKey("address");
  if (address) {
    wallet->address = *address;
  }

  auto* one_time_string = dictionary->FindStringKey("one_time_string");
  if (one_time_string) {
    wallet->one_time_string = *one_time_string;
  }

  auto* code_verifier = dictionary->FindStringKey("code_verifier");
  if (code_verifier) {
    wallet->code_verifier = *code_verifier;
  }

  auto status = dictionary->FindIntKey("status");
  if (status) {
    wallet->status = static_cast<ledger::type::WalletStatus>(*status);
  }

  auto* user_name = dictionary->FindStringKey("user_name");
  if (user_name) {
    wallet->user_name = *user_name;
  }

  auto* member_id = dictionary->FindStringKey("member_id");
  if (member_id) {
    wallet->member_id = *member_id;
  }

  auto* verify_url = dictionary->FindStringKey("verify_url");
  if (verify_url) {
    wallet->verify_url = *verify_url;
  }

  auto* add_url = dictionary->FindStringKey("add_url");
  if (add_url) {
    wallet->add_url = *add_url;
  }

  auto* withdraw_url = dictionary->FindStringKey("withdraw_url");
  if (withdraw_url) {
    wallet->withdraw_url = *withdraw_url;
  }

  auto* account_url = dictionary->FindStringKey("account_url");
  if (account_url) {
    wallet->account_url = *account_url;
  }

  auto* login_url = dictionary->FindStringKey("login_url");
  if (login_url) {
    wallet->login_url = *login_url;
  }

  auto* activity_url = dictionary->FindStringKey("activity_url");
  if (activity_url) {
    wallet->activity_url = *activity_url;
  }

  auto* fees = dictionary->FindDictKey("fees");
  if (fees) {
    base::DictionaryValue* fees_dictionary;
    if (fees->GetAsDictionary(&fees_dictionary)) {
      for (base::DictionaryValue::Iterator it(*fees_dictionary); !it.IsAtEnd();
           it.Advance()) {
        if (!it.value().is_double()) {
          continue;
        }

        wallet->fees.insert(std::make_pair(it.key(), it.value().GetDouble()));
      }
    }
  }

  return wallet;
}

type::ExternalWalletPtr GetWallet(LedgerImpl* ledger,
                                  const std::string wallet_type) {
  DCHECK(ledger);

  auto json =
      ledger->state()->GetEncryptedString(WalletTypeToState(wallet_type));

  if (!json || json->empty())
    return nullptr;

  return ExternalWalletPtrFromJSON(*json, wallet_type);
}

bool SetWallet(LedgerImpl* ledger,
               type::ExternalWalletPtr wallet,
               const std::string state) {
  DCHECK(ledger);
  if (!wallet) {
    return false;
  }

  base::Value fees(base::Value::Type::DICTIONARY);
  for (const auto& fee : wallet->fees) {
    fees.SetDoubleKey(fee.first, fee.second);
  }

  base::Value new_wallet(base::Value::Type::DICTIONARY);
  new_wallet.SetStringKey("token", wallet->token);
  new_wallet.SetStringKey("address", wallet->address);
  new_wallet.SetIntKey("status", static_cast<int>(wallet->status));
  new_wallet.SetStringKey("one_time_string", wallet->one_time_string);
  new_wallet.SetStringKey("code_verifier", wallet->code_verifier);
  new_wallet.SetStringKey("user_name", wallet->user_name);
  new_wallet.SetStringKey("member_id", wallet->member_id);
  new_wallet.SetStringKey("verify_url", wallet->verify_url);
  new_wallet.SetStringKey("add_url", wallet->add_url);
  new_wallet.SetStringKey("withdraw_url", wallet->withdraw_url);
  new_wallet.SetStringKey("account_url", wallet->account_url);
  new_wallet.SetStringKey("login_url", wallet->login_url);
  new_wallet.SetStringKey("activity_url", wallet->activity_url);
  new_wallet.SetKey("fees", std::move(fees));

  std::string json;
  base::JSONWriter::Write(new_wallet, &json);
  return ledger->state()->SetEncryptedString(state, json);
}

type::ExternalWalletPtr ResetWallet(type::ExternalWalletPtr wallet) {
  if (!wallet) {
    return nullptr;
  }

  const auto status = wallet->status;
  const auto wallet_type = wallet->type;
  DCHECK(!wallet_type.empty());
  wallet = type::ExternalWallet::New();
  wallet->type = wallet_type;

  if (wallet_type == constant::kWalletUphold) {
    if (status == type::WalletStatus::VERIFIED) {
      wallet->status = type::WalletStatus::DISCONNECTED_VERIFIED;
    }
  } else {
    if (status != type::WalletStatus::NOT_CONNECTED) {
      if (status == type::WalletStatus::VERIFIED) {
        wallet->status = type::WalletStatus::DISCONNECTED_VERIFIED;
      } else {
        wallet->status = type::WalletStatus::DISCONNECTED_NOT_VERIFIED;
      }
    }
  }

  return wallet;
}

}  // namespace wallet
}  // namespace ledger
