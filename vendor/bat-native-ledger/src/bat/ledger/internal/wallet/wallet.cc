/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/wallet.h"

#include <map>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/bitflyer/bitflyer.h"
#include "bat/ledger/internal/bitflyer/bitflyer_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/uphold/uphold.h"
#include "bat/ledger/internal/uphold/uphold_util.h"

#include "wally_bip39.h"  // NOLINT

namespace ledger {
namespace wallet {

Wallet::Wallet(LedgerImpl* ledger) :
    ledger_(ledger),
    create_(std::make_unique<WalletCreate>(ledger)),
    recover_(std::make_unique<WalletRecover>(ledger)),
    balance_(std::make_unique<WalletBalance>(ledger)),
    claim_(std::make_unique<WalletClaim>(ledger)),
    promotion_server_(std::make_unique<endpoint::PromotionServer>(ledger)) {
}

Wallet::~Wallet() = default;

void Wallet::CreateWalletIfNecessary(ledger::ResultCallback callback) {
  create_->Start(std::move(callback));
}

std::string Wallet::GetWalletPassphrase(type::BraveWalletPtr wallet) {
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return "";
  }

  if (wallet->recovery_seed.empty()) {
    BLOG(0, "Seed is empty");
    return "";
  }

  char* words = nullptr;
  const int result = bip39_mnemonic_from_bytes(
      nullptr,
      &wallet->recovery_seed.front(),
      wallet->recovery_seed.size(),
      &words);

  if (result != 0) {
    BLOG(0, "Bip39 failed: " << result);
    NOTREACHED();
    return "";
  }

  const std::string pass_phrase = words;
  wally_free_string(words);

  return pass_phrase;
}

void Wallet::RecoverWallet(
    const std::string& pass_phrase,
    ledger::ResultCallback callback) {
  recover_->Start(
      pass_phrase,
      [this, callback](const type::Result result) {
        if (result == type::Result::LEDGER_OK) {
          ledger_->database()->DeleteAllBalanceReports(
              [](const type::Result _) {});
          DisconnectAllWallets(callback);
          return;
        }
        callback(result);
      });
}

void Wallet::FetchBalance(ledger::FetchBalanceCallback callback) {
  balance_->Fetch(callback);
}

void Wallet::ExternalWalletAuthorization(
    const std::string& wallet_type,
    const base::flat_map<std::string, std::string>& args,
    ledger::ExternalWalletAuthorizationCallback callback) {
  ledger_->wallet()->CreateWalletIfNecessary(
    [this, wallet_type, args, callback](const type::Result result) {
      if (result != type::Result::WALLET_CREATED) {
        BLOG(0, "Wallet couldn't be created");
        callback(type::Result::LEDGER_ERROR, {});
        return;
      }

      AuthorizeWallet(wallet_type, args, callback);
    });
}

void Wallet::AuthorizeWallet(
    const std::string& wallet_type,
    const base::flat_map<std::string, std::string>& args,
    ledger::ExternalWalletAuthorizationCallback callback) {
  if (wallet_type == constant::kWalletUphold) {
    ledger_->uphold()->WalletAuthorization(args, callback);
    return;
  }

  if (wallet_type == constant::kWalletBitflyer) {
    ledger_->bitflyer()->WalletAuthorization(args, callback);
    return;
  }

  NOTREACHED();
  callback(type::Result::LEDGER_ERROR, {});
}

void Wallet::DisconnectWallet(
      const std::string& wallet_type,
      ledger::ResultCallback callback) {
  if (wallet_type == constant::kWalletUphold) {
    ledger_->uphold()->DisconnectWallet(true);
    callback(type::Result::LEDGER_OK);
    return;
  }

  if (wallet_type == constant::kWalletBitflyer) {
    ledger_->bitflyer()->DisconnectWallet(true);
    callback(type::Result::LEDGER_OK);
    return;
  }

  NOTREACHED();
  callback(type::Result::LEDGER_OK);
}

void Wallet::ClaimFunds(ledger::ResultCallback callback) {
  // Anonymous funds claim
  claim_->Start([this, callback](const type::Result result) {
    if (result != type::Result::LEDGER_OK &&
        result != type::Result::ALREADY_EXISTS) {
      BLOG(0, "Claiming anon funds failed");
      callback(type::Result::CONTINUE);
      return;
    }

    // tokens claim
    ledger_->promotion()->TransferTokens(
        [callback](const type::Result result) {
          if (result != type::Result::LEDGER_OK) {
            BLOG(0, "Claiming tokens failed");
            callback(type::Result::CONTINUE);
            return;
          }

          callback(type::Result::LEDGER_OK);
        });
  });
}

void Wallet::GetAnonWalletStatus(ledger::ResultCallback callback) {
  const auto wallet = GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  const std::string passphrase = GetWalletPassphrase(wallet->Clone());
  const uint64_t stamp = ledger_->state()->GetCreationStamp();

  if (!wallet->payment_id.empty() && stamp != 0) {
    callback(type::Result::WALLET_CREATED);
    return;
  }

  if (wallet->payment_id.empty() || passphrase.empty()) {
    BLOG(0, "Wallet is corrupted");
    callback(type::Result::CORRUPTED_DATA);
    return;
  }

  callback(type::Result::LEDGER_OK);
}

void Wallet::DisconnectAllWallets(ledger::ResultCallback callback) {
  DisconnectWallet(constant::kWalletUphold, [](const type::Result result) {});
  DisconnectWallet(constant::kWalletBitflyer, [](const type::Result result) {});
  callback(type::Result::LEDGER_OK);
}

type::BraveWalletPtr Wallet::GetWallet() {
  const std::string wallet_string =
      ledger_->ledger_client()->GetEncryptedStringState(state::kWalletBrave);

  if (wallet_string.empty()) {
    return nullptr;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(wallet_string);
  if (!value || !value->is_dict()) {
    BLOG(0, "Parsing of brave wallet failed");
    return nullptr;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Parsing of brave wallet failed");
    return nullptr;
  }

  auto wallet = ledger::type::BraveWallet::New();

  auto* payment_id = dictionary->FindStringKey("payment_id");
  if (!payment_id) {
    return nullptr;
  }
  wallet->payment_id = *payment_id;

  auto* seed = dictionary->FindStringKey("recovery_seed");
  if (!seed) {
    return nullptr;
  }
  std::string decoded_seed;
  if (!base::Base64Decode(*seed, &decoded_seed)) {
    BLOG(0, "Problem decoding recovery seed");
    NOTREACHED();
    return nullptr;
  }

  std::vector<uint8_t> vector_seed;
  vector_seed.assign(decoded_seed.begin(), decoded_seed.end());
  wallet->recovery_seed = vector_seed;

  return wallet;
}

bool Wallet::SetWallet(type::BraveWalletPtr wallet) {
  if (!wallet) {
    BLOG(0, "Brave wallet is null");
    return false;
  }

  const std::string seed_string = base::Base64Encode(wallet->recovery_seed);
  std::string event_string;
  if (wallet->recovery_seed.size() > 1) {
    event_string = std::to_string(
        wallet->recovery_seed[0] + wallet->recovery_seed[1]);
  }

  base::Value new_wallet(base::Value::Type::DICTIONARY);
  new_wallet.SetStringKey("payment_id", wallet->payment_id);
  new_wallet.SetStringKey("recovery_seed", seed_string);

  std::string json;
  base::JSONWriter::Write(new_wallet, &json);
  const bool success = ledger_->ledger_client()->SetEncryptedStringState(
      state::kWalletBrave,
      json);
  ledger_->database()->SaveEventLog(state::kRecoverySeed, event_string);
  if (!wallet->payment_id.empty()) {
    ledger_->database()->SaveEventLog(state::kPaymentId, wallet->payment_id);
  }

  BLOG_IF(0, !success, "Can't encrypt brave wallet");

  return success;
}

void Wallet::LinkBraveWallet(
    const std::string& destination_payment_id,
    ledger::ResultCallback callback) {
  promotion_server_->post_claim_brave()->Request(
      destination_payment_id,
      [this, callback](const type::Result result) {
        if (result != type::Result::LEDGER_OK &&
            result != type::Result::ALREADY_EXISTS) {
          callback(result);
          return;
        }

        ledger_->promotion()->TransferTokens(callback);
      });
}

}  // namespace wallet
}  // namespace ledger
