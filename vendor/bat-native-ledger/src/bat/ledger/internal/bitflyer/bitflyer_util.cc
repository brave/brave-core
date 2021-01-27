/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/bitflyer/bitflyer_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "crypto/random.h"

namespace ledger {
namespace bitflyer {

std::string GetClientId() {
  return ledger::_environment == type::Environment::PRODUCTION
             ? kClientIdProduction
             : kClientIdStaging;
}

std::string GetUrl() {
  return ledger::_environment == type::Environment::PRODUCTION ? kUrlProduction
                                                               : kUrlStaging;
}

std::string GetFeeAddress() {
  return ledger::_environment == type::Environment::PRODUCTION
             ? kFeeAddressProduction
             : kFeeAddressStaging;
}

std::string GetACAddress() {
  return ledger::_environment == type::Environment::PRODUCTION
             ? kACAddressProduction
             : kACAddressStaging;
}

std::string GetAuthorizeUrl(const std::string& state) {
  const std::string id = GetClientId();
  const std::string url = GetUrl();

  return base::StringPrintf(
      "%s/ex/OAuth/authorize"
      "?client_id=%s"
      "&scope="
      "assets "
      "create_deposit_id "
      "withdraw_to_deposit_id"
      "&redirect_uri=rewards://bitflyer/authorization"
      "&state=%s"
      "&response_type=code",
      url.c_str(), id.c_str(), state.c_str());
}

std::string GetAddUrl() {
  return GetAccountUrl();
}

std::string GetWithdrawUrl() {
  return GetAccountUrl();
}

type::ExternalWalletPtr GetWallet(LedgerImpl* ledger) {
  DCHECK(ledger);
  const std::string wallet_string =
      ledger->ledger_client()->GetEncryptedStringState(state::kWalletBitflyer);

  if (wallet_string.empty()) {
    return nullptr;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(wallet_string);
  if (!value || !value->is_dict()) {
    BLOG(0, "Parsing of Bitflyer wallet failed");
    return nullptr;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Parsing of Bitflyer wallet failed");
    return nullptr;
  }

  auto wallet = ledger::type::ExternalWallet::New();
  wallet->type = constant::kWalletBitflyer;

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

  auto status = dictionary->FindIntKey("status");
  if (status) {
    wallet->status = static_cast<ledger::type::WalletStatus>(*status);
  }

  auto* user_name = dictionary->FindStringKey("user_name");
  if (user_name) {
    wallet->user_name = *user_name;
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

bool SetWallet(LedgerImpl* ledger, type::ExternalWalletPtr wallet) {
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
  new_wallet.SetStringKey("user_name", wallet->user_name);
  new_wallet.SetStringKey("verify_url", wallet->verify_url);
  new_wallet.SetStringKey("add_url", wallet->add_url);
  new_wallet.SetStringKey("withdraw_url", wallet->withdraw_url);
  new_wallet.SetStringKey("account_url", wallet->account_url);
  new_wallet.SetStringKey("login_url", wallet->login_url);
  new_wallet.SetKey("fees", std::move(fees));

  std::string json;
  base::JSONWriter::Write(new_wallet, &json);
  const bool success = ledger->ledger_client()->SetEncryptedStringState(
      state::kWalletBitflyer, json);

  BLOG_IF(0, !success, "Can't encrypt Bitflyer wallet");

  return success;
}

std::string GenerateRandomString(bool testing) {
  if (testing) {
    return "123456789";
  }

  const size_t kLength = 32;
  uint8_t bytes[kLength];
  crypto::RandBytes(bytes, sizeof(bytes));
  return base::HexEncode(bytes, sizeof(bytes));
}

std::string GetAccountUrl() {
  const std::string url = GetUrl();

  return base::StringPrintf("%s/ex/Home?login=1", url.c_str());
}

type::ExternalWalletPtr GenerateLinks(type::ExternalWalletPtr wallet) {
  if (!wallet) {
    return nullptr;
  }

  switch (wallet->status) {
    case type::WalletStatus::VERIFIED: {
      wallet->add_url = GetAddUrl();
      wallet->withdraw_url = GetWithdrawUrl();
      break;
    }
    case type::WalletStatus::CONNECTED:
    case type::WalletStatus::PENDING:
    case type::WalletStatus::NOT_CONNECTED:
    case type::WalletStatus::DISCONNECTED_VERIFIED:
    case type::WalletStatus::DISCONNECTED_NOT_VERIFIED: {
      wallet->add_url = "";
      wallet->withdraw_url = "";
      break;
    }
  }

  wallet->verify_url = GetAuthorizeUrl(wallet->one_time_string);
  wallet->account_url = GetAccountUrl();
  wallet->login_url = GetAuthorizeUrl(wallet->one_time_string);

  return wallet;
}

type::ExternalWalletPtr ResetWallet(type::ExternalWalletPtr wallet) {
  if (!wallet) {
    return nullptr;
  }

  const auto status = wallet->status;
  wallet = type::ExternalWallet::New();
  wallet->type = constant::kWalletBitflyer;

  if (status != type::WalletStatus::NOT_CONNECTED) {
    if (status == type::WalletStatus::VERIFIED) {
      wallet->status = type::WalletStatus::DISCONNECTED_VERIFIED;
    } else {
      wallet->status = type::WalletStatus::DISCONNECTED_NOT_VERIFIED;
    }
  }

  return wallet;
}

}  // namespace bitflyer
}  // namespace ledger
