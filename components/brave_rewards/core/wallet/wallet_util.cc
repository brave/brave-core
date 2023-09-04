/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet/wallet_util.h"

#include <algorithm>
#include <utility>

#include "base/functional/overloaded.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/bitflyer/bitflyer.h"
#include "brave/components/brave_rewards/core/bitflyer/bitflyer_util.h"
#include "brave/components/brave_rewards/core/common/random_util.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/gemini/gemini.h"
#include "brave/components/brave_rewards/core/gemini/gemini_util.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/logging/event_log_keys.h"
#include "brave/components/brave_rewards/core/notifications/notification_keys.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/state/state.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/uphold/uphold.h"
#include "brave/components/brave_rewards/core/uphold/uphold_util.h"
#include "brave/components/brave_rewards/core/zebpay/zebpay_util.h"

namespace brave_rewards::internal::wallet {

namespace {

std::string WalletTypeToState(const std::string& wallet_type) {
  if (wallet_type == constant::kWalletBitflyer) {
    return state::kWalletBitflyer;
  } else if (wallet_type == constant::kWalletGemini) {
    return state::kWalletGemini;
  } else if (wallet_type == constant::kWalletUphold) {
    return state::kWalletUphold;
  } else if (wallet_type == constant::kWalletZebPay) {
    return state::kWalletZebPay;
  } else if (wallet_type == "test") {
    return "wallets." + wallet_type;
  } else {
    NOTREACHED() << "Unexpected wallet type " << wallet_type << '!';
    return "";
  }
}

void OnWalletStatusChange(RewardsEngineImpl& engine,
                          const std::string& wallet_type,
                          absl::optional<mojom::WalletStatus> from,
                          mojom::WalletStatus to) {
  std::ostringstream oss{};
  if (from) {
    oss << *from << ' ';
  }
  oss << "==> " << to;

  engine.database()->SaveEventLog(log::kWalletStatusChange,
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
  auto wallet = mojom::ExternalWallet::New();
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
    wallet->status = static_cast<mojom::WalletStatus>(*status);
  }

  const auto* user_name = dict.FindString("user_name");
  if (user_name) {
    wallet->user_name = *user_name;
  }

  const auto* member_id = dict.FindString("member_id");
  if (member_id) {
    wallet->member_id = *member_id;
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

mojom::ExternalWalletPtr GetWallet(RewardsEngineImpl& engine,
                                   const std::string& wallet_type) {
  const auto state = WalletTypeToState(wallet_type);
  if (state.empty()) {
    return nullptr;
  }

  auto json = engine.state()->GetEncryptedString(state);
  if (!json || json->empty()) {
    return nullptr;
  }

  return ExternalWalletPtrFromJSON(*json, wallet_type);
}

mojom::ExternalWalletPtr GetWalletIf(
    RewardsEngineImpl& engine,
    const std::string& wallet_type,
    const std::set<mojom::WalletStatus>& statuses) {
  if (statuses.empty()) {
    return nullptr;
  }

  auto wallet = GetWallet(engine, wallet_type);
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

bool SetWallet(RewardsEngineImpl& engine, mojom::ExternalWalletPtr wallet) {
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
  new_wallet.Set("account_url", wallet->account_url);
  new_wallet.Set("login_url", wallet->login_url);
  new_wallet.Set("activity_url", wallet->activity_url);
  new_wallet.Set("fees", std::move(fees));

  std::string json;
  if (!base::JSONWriter::Write(std::move(new_wallet), &json)) {
    return false;
  }

  return engine.state()->SetEncryptedString(WalletTypeToState(wallet->type),
                                            json);
}

// Valid transition:
// - ==> kNotConnected:
//   - on wallet creation
//
// Invariants:
// - kNotConnected: token and address are cleared
mojom::ExternalWalletPtr EnsureValidCreation(const std::string& wallet_type,
                                             mojom::WalletStatus to) {
  if (to != mojom::WalletStatus::kNotConnected) {
    BLOG(0, "Attempting to create " << wallet_type << " wallet as " << to
                                    << " (a status other than "
                                       "kNotConnected)!");
    return nullptr;
  }

  auto wallet = mojom::ExternalWallet::New();
  wallet->type = wallet_type;

  wallet->one_time_string = util::GenerateRandomHexString();
  wallet->code_verifier = util::GeneratePKCECodeVerifier();

  wallet->status = to;

  return wallet;
}

// Valid transitions:
// - kNotConnected ==> kConnected:
//    - on successful wallet connection
// - kConnected ==> kNotConnected:
//    - on getting notified of linkage termination on the server side
// - kConnected ==> kLoggedOut:
//    - on access token expiry
//    - on losing eligibility for wallet connection (Uphold-only)
// - kLoggedOut ==> kNotConnected:
//    - on getting notified of linkage termination on the server side
// - kLoggedOut ==> kConnected:
//    - on successful (re)connection
//
// Invariants:
// - kNotConnected: token and address are cleared
// - kConnected: needs !token.empty() && !address.empty()
// - kLoggedOut: token and address are cleared
mojom::ExternalWalletPtr EnsureValidTransition(mojom::ExternalWalletPtr wallet,
                                               mojom::WalletStatus to) {
  DCHECK(wallet);
  const auto wallet_type = wallet->type;
  const auto from = wallet->status;

  // kNotConnected ==> kConnected
  const bool wallet_connection = from == mojom::WalletStatus::kNotConnected &&
                                 to == mojom::WalletStatus::kConnected;
  // kConnected ==> kLoggedOut
  const bool wallet_logout = from == mojom::WalletStatus::kConnected &&
                             to == mojom::WalletStatus::kLoggedOut;
  // kLoggedOut ==> kConnected
  const bool wallet_reconnection = from == mojom::WalletStatus::kLoggedOut &&
                                   to == mojom::WalletStatus::kConnected;
  // kConnected ==> kNotConnected || kLoggedOut ==> kNotConnected
  const bool linkage_termination = (from == mojom::WalletStatus::kConnected &&
                                    to == mojom::WalletStatus::kNotConnected) ||
                                   (from == mojom::WalletStatus::kLoggedOut &&
                                    to == mojom::WalletStatus::kNotConnected);

  if (!wallet_connection && !wallet_logout && !wallet_reconnection &&
      !linkage_termination) {
    BLOG(0, "Invalid " << wallet_type << " wallet status transition: " << from
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
      // token.empty() && address.empty()
      wallet = mojom::ExternalWallet::New();
      wallet->type = wallet_type;

      wallet->one_time_string = util::GenerateRandomHexString();
      wallet->code_verifier = util::GeneratePKCECodeVerifier();
      break;
  }

  wallet->status = to;

  return wallet;
}

mojom::ExternalWalletPtr TransitionWallet(
    RewardsEngineImpl& engine,
    absl::variant<mojom::ExternalWalletPtr, std::string> wallet_info,
    mojom::WalletStatus to) {
  absl::optional<mojom::WalletStatus> from;

  auto wallet = absl::visit(
      base::Overloaded{
          [&](const std::string& wallet_type) -> mojom::ExternalWalletPtr {
            auto wallet = GetWallet(engine, wallet_type);
            if (wallet) {
              BLOG(0, wallet_type << " wallet already exists!");
              return nullptr;
            }

            return EnsureValidCreation(wallet_type, to);
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

  wallet = GenerateLinks(std::move(wallet));
  if (!wallet) {
    BLOG(0, "Failed to generate links for wallet!");
    return nullptr;
  }

  if (!SetWallet(engine, wallet->Clone())) {
    BLOG(0, "Failed to set " << wallet->type << " wallet!");
    return nullptr;
  }

  OnWalletStatusChange(engine, wallet->type, from, to);

  return wallet;
}

mojom::ExternalWalletPtr MaybeCreateWallet(RewardsEngineImpl& engine,
                                           const std::string& wallet_type) {
  auto wallet = GetWallet(engine, wallet_type);
  if (!wallet) {
    wallet = TransitionWallet(engine, wallet_type,
                              mojom::WalletStatus::kNotConnected);
    if (!wallet) {
      BLOG(0, "Failed to create " << wallet_type << " wallet!");
    }
  }

  return wallet;
}

bool LogOutWallet(RewardsEngineImpl& engine,
                  const std::string& wallet_type,
                  const std::string& notification) {
  DCHECK(!wallet_type.empty());

  BLOG(1, "Logging out " << wallet_type << " wallet...");

  auto wallet =
      GetWalletIf(engine, wallet_type, {mojom::WalletStatus::kConnected});
  if (!wallet) {
    return false;
  }

  const std::string abbreviated_address = wallet->address.substr(0, 5);
  wallet = TransitionWallet(engine, std::move(wallet),
                            mojom::WalletStatus::kLoggedOut);
  if (!wallet) {
    return false;
  }

  engine.database()->SaveEventLog(log::kWalletDisconnected,
                                  wallet_type + abbreviated_address);

  if (!engine.IsShuttingDown()) {
    engine.client()->ExternalWalletLoggedOut();
    engine.client()->ShowNotification(notification.empty()
                                          ? notifications::kWalletDisconnected
                                          : notification,
                                      {}, base::DoNothing());
  }

  return true;
}

mojom::ExternalWalletPtr GenerateLinks(mojom::ExternalWalletPtr wallet) {
  if (wallet->type == constant::kWalletBitflyer) {
    return bitflyer::GenerateLinks(std::move(wallet));
  } else if (wallet->type == constant::kWalletGemini) {
    return gemini::GenerateLinks(std::move(wallet));
  } else if (wallet->type == constant::kWalletUphold) {
    return uphold::GenerateLinks(std::move(wallet));
  } else if (wallet->type == constant::kWalletZebPay) {
    return zebpay::GenerateLinks(std::move(wallet));
  } else if (wallet->type == "test") {
    return wallet;
  } else {
    NOTREACHED() << "Unexpected wallet type " << wallet->type << '!';
    return nullptr;
  }
}

void FetchBalance(RewardsEngineImpl& engine,
                  const std::string& wallet_type,
                  base::OnceCallback<void(mojom::Result, double)> callback) {
  if (wallet_type == constant::kWalletBitflyer) {
    engine.bitflyer()->FetchBalance(std::move(callback));
  } else if (wallet_type == constant::kWalletGemini) {
    engine.gemini()->FetchBalance(std::move(callback));
  } else if (wallet_type == constant::kWalletUphold) {
    engine.uphold()->FetchBalance(std::move(callback));
  } else if (wallet_type == constant::kWalletZebPay) {
    engine.zebpay()->FetchBalance(std::move(callback));
  } else {
    NOTREACHED() << "Unexpected wallet type " << wallet_type << '!';
    std::move(callback).Run(mojom::Result::FAILED, 0.0);
  }
}

}  // namespace brave_rewards::internal::wallet
