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
#include "bat/ledger/internal/logging/event_log_keys.h"
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

mojom::ExternalWalletPtr ExternalWalletPtrFromJSON(std::string wallet_string,
                                                   std::string wallet_type) {
  absl::optional<base::Value> value = base::JSONReader::Read(wallet_string);
  if (!value || !value->is_dict()) {
    BLOG(0, "Parsing of " + wallet_type + " wallet failed");
    return nullptr;
  }

  const base::Value::Dict& dict = value->GetDict();
  auto wallet = ledger::mojom::ExternalWallet::New();
  wallet->type = wallet_type;

  const auto* token = dict.FindString("token");
  if (token) {
    wallet->token = *token;
  }

  const auto* address = dict.FindString("address");
  if (address) {
    wallet->address = *address;
  }

  const auto* one_time_string = dict.FindString("one_time_string");
  if (one_time_string) {
    wallet->one_time_string = *one_time_string;
  }

  const auto* code_verifier = dict.FindString("code_verifier");
  if (code_verifier) {
    wallet->code_verifier = *code_verifier;
  }

  auto status = dict.FindInt("status");
  if (status) {
    wallet->status = static_cast<ledger::mojom::WalletStatus>(*status);
  }

  const auto* user_name = dict.FindString("user_name");
  if (user_name) {
    wallet->user_name = *user_name;
  }

  const auto* member_id = dict.FindString("member_id");
  if (member_id) {
    wallet->member_id = *member_id;
  }

  const auto* add_url = dict.FindString("add_url");
  if (add_url) {
    wallet->add_url = *add_url;
  }

  const auto* withdraw_url = dict.FindString("withdraw_url");
  if (withdraw_url) {
    wallet->withdraw_url = *withdraw_url;
  }

  const auto* account_url = dict.FindString("account_url");
  if (account_url) {
    wallet->account_url = *account_url;
  }

  auto* login_url = dict.FindString("login_url");
  if (login_url) {
    wallet->login_url = *login_url;
  }

  const auto* activity_url = dict.FindString("activity_url");
  if (activity_url) {
    wallet->activity_url = *activity_url;
  }

  if (const auto* fees = dict.FindDict("fees")) {
    for (const auto [k, v] : *fees) {
      if (!v.is_double()) {
        continue;
      }

      wallet->fees.insert(std::make_pair(k, v.GetDouble()));
    }
  }

  return wallet;
}

mojom::ExternalWalletPtr GetWallet(LedgerImpl* ledger,
                                   const std::string wallet_type) {
  DCHECK(ledger);

  auto json =
      ledger->state()->GetEncryptedString(WalletTypeToState(wallet_type));

  if (!json || json->empty())
    return nullptr;

  return ExternalWalletPtrFromJSON(*json, wallet_type);
}

bool SetWallet(LedgerImpl* ledger,
               mojom::ExternalWalletPtr wallet,
               const std::string state) {
  DCHECK(ledger);
  if (!wallet) {
    return false;
  }

  base::Value::Dict fees;
  for (const auto& fee : wallet->fees) {
    fees.Set(fee.first, fee.second);
  }

  base::Value::Dict new_wallet;
  new_wallet.Set("token", wallet->token);
  new_wallet.Set("address", wallet->address);
  new_wallet.Set("status", static_cast<int>(wallet->status));
  new_wallet.Set("one_time_string", wallet->one_time_string);
  new_wallet.Set("code_verifier", wallet->code_verifier);
  new_wallet.Set("user_name", wallet->user_name);
  new_wallet.Set("member_id", wallet->member_id);
  new_wallet.Set("add_url", wallet->add_url);
  new_wallet.Set("withdraw_url", wallet->withdraw_url);
  new_wallet.Set("account_url", wallet->account_url);
  new_wallet.Set("login_url", wallet->login_url);
  new_wallet.Set("activity_url", wallet->activity_url);
  new_wallet.Set("fees", std::move(fees));

  std::string json;
  base::JSONWriter::Write(new_wallet, &json);
  return ledger->state()->SetEncryptedString(state, json);
}

mojom::ExternalWalletPtr ResetWallet(mojom::ExternalWalletPtr wallet) {
  if (!wallet) {
    return nullptr;
  }

  const auto status = wallet->status;
  const auto wallet_type = wallet->type;
  DCHECK(!wallet_type.empty());
  wallet = mojom::ExternalWallet::New();
  wallet->type = wallet_type;

  if (wallet_type == constant::kWalletUphold) {
    if (status == mojom::WalletStatus::VERIFIED) {
      wallet->status = mojom::WalletStatus::DISCONNECTED_VERIFIED;
    }
  } else {
    if (status != mojom::WalletStatus::NOT_CONNECTED) {
      if (status == mojom::WalletStatus::VERIFIED) {
        wallet->status = mojom::WalletStatus::DISCONNECTED_VERIFIED;
      } else {
        wallet->status = mojom::WalletStatus::DISCONNECTED_NOT_VERIFIED;
      }
    }
  }

  return wallet;
}

template <typename T, typename... Ts>
bool one_of(T&& t, Ts&&... ts) {
  bool match = false;

  static_cast<void>(std::initializer_list<bool>{
      (match = match || std::forward<T>(t) == std::forward<Ts>(ts))...});

  return match;
}

void OnWalletStatusChange(LedgerImpl* ledger,
                          absl::optional<mojom::WalletStatus> from,
                          mojom::WalletStatus to) {
  DCHECK(ledger);
  DCHECK(!from ||
         one_of(*from, mojom::WalletStatus::NOT_CONNECTED,
                mojom::WalletStatus::DISCONNECTED_VERIFIED,
                mojom::WalletStatus::PENDING, mojom::WalletStatus::VERIFIED));
  DCHECK(one_of(to, mojom::WalletStatus::NOT_CONNECTED,
                mojom::WalletStatus::DISCONNECTED_VERIFIED,
                mojom::WalletStatus::PENDING, mojom::WalletStatus::VERIFIED));

  std::ostringstream oss{};
  if (from) {
    oss << *from << ' ';
  }
  oss << "==> " << to;

  ledger->database()->SaveEventLog(log::kWalletStatusChange, oss.str());
}

}  // namespace wallet
}  // namespace ledger
