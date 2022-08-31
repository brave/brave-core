/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet_provider/wallet_provider.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"

namespace ledger {

WalletProvider::WalletProvider(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);

  if (auto wallet = GetWallet()) {
    previous_status = wallet->status;
  }
}

WalletProvider::~WalletProvider() = default;

type::ExternalWalletPtr WalletProvider::GetWallet() const {
  const auto wallet_string =
      ledger_->state()->GetEncryptedString(std::string{"wallets."} + Name());

  if (!wallet_string || wallet_string->empty()) {
    return nullptr;
  }

  const auto wallet_json = base::JSONReader::Read(*wallet_string);
  if (!wallet_json || !wallet_json->is_dict()) {
    BLOG(0, "Failed to parse " << Name() << " wallet!");
    return nullptr;
  }

  const base::Value::Dict& dict = wallet_json->GetDict();
  auto wallet = ledger::type::ExternalWallet::New();
  wallet->type = Name();

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

  const auto status = dict.FindInt("status");
  if (status) {
    wallet->status = static_cast<ledger::type::WalletStatus>(*status);
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

  const auto* login_url = dict.FindString("login_url");
  if (login_url) {
    wallet->login_url = *login_url;
  }

  const auto* activity_url = dict.FindString("activity_url");
  if (activity_url) {
    wallet->activity_url = *activity_url;
  }

  if (const auto* fees = dict.FindDict("fees")) {
    for (const auto [key, value] : *fees) {
      if (!value.is_double()) {
        continue;
      }

      wallet->fees.emplace(key, value.GetDouble());
    }
  }

  return wallet;
}

bool WalletProvider::SetWallet(type::ExternalWalletPtr wallet) {
  if (!wallet) {
    return false;
  }

  base::Value::Dict fees;
  for (auto [contribution_id, amount] : wallet->fees) {
    fees.Set(std::move(contribution_id), amount);
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

  std::string wallet_json;
  if (!base::JSONWriter::Write(new_wallet, &wallet_json)) {
    return false;
  }

  const bool success = ledger_->state()->SetEncryptedString(
      std::string{"wallets."} + Name(), wallet_json);

  if (success) {
    LogWalletStatusChange(previous_status, wallet->status);
    previous_status = wallet->status;
  }

  return success;
}

void WalletProvider::DisconnectWallet(
    const absl::optional<std::string>& notification) {
  auto wallet = GetWallet();
  if (!wallet) {
    return;
  }

  BLOG(1, "Disconnecting wallet");
  const std::string wallet_address = wallet->address;

  const bool manual = !notification.has_value();

  wallet = ResetWallet(std::move(wallet));
  if (manual) {
    wallet->status = type::WalletStatus::NOT_CONNECTED;
  }

  const bool shutting_down = ledger_->IsShuttingDown();

  if (!manual && !shutting_down && !notification->empty()) {
    ledger_->ledger_client()->ShowNotification(*notification, {"Uphold"},
                                               [](type::Result) {});
  }

  wallet = GenerateLinks(std::move(wallet));
  SetWallet(std::move(wallet));

  if (!shutting_down) {
    ledger_->ledger_client()->WalletDisconnected(Name());
  }

  ledger_->database()->SaveEventLog(log::kWalletDisconnected,
                                    std::string(Name()) +
                                        (!wallet_address.empty() ? "/" : "") +
                                        wallet_address.substr(0, 5));
}

type::ExternalWalletPtr WalletProvider::ResetWallet(
    type::ExternalWalletPtr wallet) {
  if (!wallet) {
    return nullptr;
  }

  const auto previous_status = wallet->status;
  wallet = type::ExternalWallet::New();
  wallet->type = Name();

  if (previous_status != type::WalletStatus::NOT_CONNECTED) {
    if (previous_status == type::WalletStatus::VERIFIED) {
      wallet->status = type::WalletStatus::DISCONNECTED_VERIFIED;
    } else {
      wallet->status = type::WalletStatus::DISCONNECTED_NOT_VERIFIED;
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

void WalletProvider::LogWalletStatusChange(
    absl::optional<type::WalletStatus> from,
    type::WalletStatus to) const {
  DCHECK(!from ||
         one_of(*from, type::WalletStatus::NOT_CONNECTED,
                type::WalletStatus::DISCONNECTED_VERIFIED,
                type::WalletStatus::PENDING, type::WalletStatus::VERIFIED));
  DCHECK(one_of(to, type::WalletStatus::NOT_CONNECTED,
                type::WalletStatus::DISCONNECTED_VERIFIED,
                type::WalletStatus::PENDING, type::WalletStatus::VERIFIED));

  std::ostringstream oss{};
  if (from) {
    oss << *from << ' ';
  }
  oss << "==> " << to;

  ledger_->database()->SaveEventLog(log::kWalletStatusChange, oss.str());
}

}  // namespace ledger
