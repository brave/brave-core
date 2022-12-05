/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/wallet_util.h"

#include <algorithm>
#include <utility>

#include "base/functional/overloaded.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/bitflyer/bitflyer_util.h"
#include "bat/ledger/internal/common/random_util.h"
#include "bat/ledger/internal/gemini/gemini_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"
#include "bat/ledger/internal/notifications/notification_keys.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/uphold/uphold_util.h"

namespace ledger::wallet {

namespace {

std::string WalletTypeToState(const std::string& wallet_type) {
  if (wallet_type == constant::kWalletBitflyer) {
    return state::kWalletBitflyer;
  } else if (wallet_type == constant::kWalletGemini) {
    return state::kWalletGemini;
  } else if (wallet_type == constant::kWalletUphold) {
    return state::kWalletUphold;
  } else if (wallet_type == "test") {
    return "wallets." + wallet_type;
  } else {
    NOTREACHED() << "Unexpected wallet type " << wallet_type << '!';
    return "";
  }
}

void OnWalletStatusChange(LedgerImpl* ledger,
                          const std::string& wallet_type,
                          absl::optional<mojom::WalletStatus> from,
                          mojom::WalletStatus to) {
  DCHECK(ledger);

  std::ostringstream oss{};
  if (from) {
    oss << *from << ' ';
  }
  oss << "==> " << to;

  ledger->database()->SaveEventLog(log::kWalletStatusChange,
                                   oss.str() + " (" + wallet_type + ')');
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
                                   const std::string& wallet_type) {
  DCHECK(ledger);

  auto json =
      ledger->state()->GetEncryptedString(WalletTypeToState(wallet_type));

  if (!json || json->empty())
    return nullptr;

  return ExternalWalletPtrFromJSON(*json, wallet_type);
}

mojom::ExternalWalletPtr GetWalletIf(
    LedgerImpl* ledger,
    const std::string& wallet_type,
    const std::set<mojom::WalletStatus>& statuses) {
  if (statuses.empty()) {
    return nullptr;
  }

  auto wallet = GetWallet(ledger, wallet_type);
  if (!wallet) {
    BLOG(9, wallet_type << " wallet is null!");
    return nullptr;
  }

  if (!statuses.count(wallet->status)) {
    std::ostringstream oss;
    auto cend = statuses.cend();
    std::copy(statuses.cbegin(), --cend,
              std::ostream_iterator<mojom::WalletStatus>(oss, ", "));
    oss << *cend;

    BLOG(9, "Unexpected state for " << wallet_type << " wallet (currently in "
                                    << wallet->status
                                    << ", expected was: " << oss.str() << ")!");
    return nullptr;
  }

  return wallet;
}

bool SetWallet(LedgerImpl* ledger, mojom::ExternalWalletPtr wallet) {
  DCHECK(ledger);

  if (!wallet || wallet->type.empty()) {
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
  if (!base::JSONWriter::Write(std::move(new_wallet), &json)) {
    return false;
  }

  return ledger->state()->SetEncryptedString(WalletTypeToState(wallet->type),
                                             json);
}

// Valid transitions:
// - kNotConnected ==> kConnected:
//    - on successful wallet connection
// - kConnected ==> kNotConnected:
//    - on manual disconnection
//    - on losing eligibility for wallet connection (Uphold-only)
// - kConnected ==> kLoggedOut:
//    - on access token expiry
// - kLoggedOut ==> kNotConnected:
//    - on manual disconnection
//    - on losing eligibility for wallet connection (Uphold-only)
// - kLoggedOut ==> kConnected:
//    - on successful (re)connection
//
// Invariants:
// - kNotConnected, kLoggedOut: token and address are cleared
// - kConnected: needs !token.empty() && !address.empty()
mojom::ExternalWalletPtr EnsureValidTransition(mojom::ExternalWalletPtr wallet,
                                               mojom::WalletStatus to) {
  const auto from = wallet->status;

  if ((from == mojom::WalletStatus::kNotConnected &&
       to != mojom::WalletStatus::kConnected) ||
      (from == mojom::WalletStatus::kConnected &&
       !std::set{mojom::WalletStatus::kNotConnected,
                 mojom::WalletStatus::kLoggedOut}
            .count(to)) ||
      (from == mojom::WalletStatus::kLoggedOut &&
       !std::set{mojom::WalletStatus::kNotConnected,
                 mojom::WalletStatus::kConnected}
            .count(to))) {
    BLOG(0, "Invalid " << wallet->type << " wallet status transition: " << from
                       << " ==> " << to << '!');
    return nullptr;
  }

  switch (to) {
    case mojom::WalletStatus::kConnected:
      if (wallet->token.empty() || wallet->address.empty()) {
        BLOG(0, "Invariant violation when attempting to transition "
                    << wallet->type << " wallet status (" << from << " ==> "
                    << to << ")!");
        return nullptr;
      }

      break;
    case mojom::WalletStatus::kNotConnected:
    case mojom::WalletStatus::kLoggedOut:
      wallet->token = "";
      wallet->address = "";
      break;
  }

  return wallet;
}

mojom::ExternalWalletPtr TransitionWallet(
    LedgerImpl* ledger,
    absl::variant<mojom::ExternalWalletPtr, std::string> wallet_info,
    mojom::WalletStatus to) {
  DCHECK(ledger);

  absl::optional<mojom::WalletStatus> from;

  auto wallet = absl::visit(
      base::Overloaded{
          [&](const std::string& wallet_type) -> mojom::ExternalWalletPtr {
            auto wallet = GetWallet(ledger, wallet_type);
            if (!wallet) {  // create
              if (to != mojom::WalletStatus::kNotConnected) {
                BLOG(0, "Attempting to create " << wallet_type << " wallet as "
                                                << to
                                                << " (a status other than "
                                                   "kNotConnected)!");
                return nullptr;
              }
            } else {  // reset
              from = wallet->status;

              if (!std::set{mojom::WalletStatus::kNotConnected,
                            mojom::WalletStatus::kLoggedOut}
                       .count(to)) {
                BLOG(0, "Attempting to reset "
                            << wallet_type << " wallet from " << *from << " to "
                            << to
                            << " (a status other than "
                               "kNotConnected, or kLoggedOut)!");
                return nullptr;
              }
            }

            wallet = mojom::ExternalWallet::New();
            wallet->type = wallet_type;

            wallet->one_time_string = util::GenerateRandomHexString();
            wallet->code_verifier = util::GeneratePKCECodeVerifier();

            return wallet;
          },
          [&](mojom::ExternalWalletPtr wallet) -> mojom::ExternalWalletPtr {
            DCHECK(wallet);
            if (!wallet) {
              BLOG(0, "Wallet is null!");
              return nullptr;
            }

            from = wallet->status;

            return EnsureValidTransition(std::move(wallet), to);
          }},
      std::move(wallet_info));

  if (!wallet) {
    return nullptr;
  }

  wallet->status = to;

  wallet = GenerateLinks(std::move(wallet));
  if (!wallet) {
    BLOG(0, "Failed to generate links for wallet!");
    return nullptr;
  }

  if (!SetWallet(ledger, wallet->Clone())) {
    BLOG(0, "Failed to set " << wallet->type << " wallet!");
    return nullptr;
  }

  OnWalletStatusChange(ledger, wallet->type, from, to);

  return wallet;
}

mojom::ExternalWalletPtr MaybeCreateWallet(LedgerImpl* ledger,
                                           const std::string& wallet_type) {
  auto wallet = GetWallet(ledger, wallet_type);
  if (!wallet) {
    wallet = TransitionWallet(ledger, wallet_type,
                              mojom::WalletStatus::kNotConnected);
    if (!wallet) {
      BLOG(0, "Failed to create " << wallet_type << " wallet!");
    }
  }

  return wallet;
}

bool DisconnectWallet(LedgerImpl* ledger,
                      const std::string& wallet_type,
                      bool manual) {
  DCHECK(ledger);
  DCHECK(!wallet_type.empty());

  BLOG(1, "Disconnecting " << wallet_type << " wallet.");

  auto wallet = GetWallet(ledger, wallet_type);
  if (!wallet) {
    return false;
  }

  auto to = mojom::WalletStatus::kNotConnected;
  if (!manual && wallet->status == mojom::WalletStatus::kConnected) {
    to = mojom::WalletStatus::kLoggedOut;
  }

  if (!TransitionWallet(ledger, wallet_type, to)) {
    return false;
  }

  ledger->database()->SaveEventLog(log::kWalletDisconnected,
                                   wallet_type +
                                       (!wallet->address.empty() ? "/" : "") +
                                       wallet->address.substr(0, 5));

  if (!ledger->IsShuttingDown()) {
    ledger->ledger_client()->WalletDisconnected(wallet_type);

    if (!manual) {
      ledger->ledger_client()->ShowNotification(
          ledger::notifications::kWalletDisconnected, {}, [](mojom::Result) {});
    }
  }

  return true;
}

mojom::ExternalWalletPtr GenerateLinks(mojom::ExternalWalletPtr wallet) {
  if (wallet->type == constant::kWalletBitflyer) {
    return ledger::bitflyer::GenerateLinks(std::move(wallet));
  } else if (wallet->type == constant::kWalletGemini) {
    return ledger::gemini::GenerateLinks(std::move(wallet));
  } else if (wallet->type == constant::kWalletUphold) {
    return ledger::uphold::GenerateLinks(std::move(wallet));
  } else if (wallet->type == "test") {
    return wallet;
  } else {
    NOTREACHED() << "Unexpected wallet type " << wallet->type << '!';
    return nullptr;
  }
}

}  // namespace ledger::wallet
