/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/wallet.h"

#include <map>
#include <utility>

#include "base/values.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/uphold/uphold.h"
#include "bat/ledger/internal/wallet/wallet_util.h"

#include "wally_bip39.h"  // NOLINT

namespace braveledger_wallet {

Wallet::Wallet(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger),
    create_(std::make_unique<WalletCreate>(ledger)),
    recover_(std::make_unique<WalletRecover>(ledger)),
    balance_(std::make_unique<WalletBalance>(ledger)),
    claim_(std::make_unique<WalletClaim>(ledger)),
    uphold_(std::make_unique<braveledger_uphold::Uphold>(ledger)) {
}

Wallet::~Wallet() = default;

void Wallet::CreateWalletIfNecessary(ledger::ResultCallback callback) {
  create_->Start(std::move(callback));
}

std::string Wallet::GetWalletPassphrase() const {
  const auto seed = ledger_->state()->GetRecoverySeed();
  std::string pass_phrase;
  if (seed.empty()) {
    BLOG(0, "Seed is empty");
    return pass_phrase;
  }

  char* words = nullptr;
  const int result = bip39_mnemonic_from_bytes(
      nullptr,
      &seed.front(),
      seed.size(),
      &words);

  if (result != 0) {
    BLOG(0, "Bip39 failed: " << result);
    NOTREACHED();
    return pass_phrase;
  }

  pass_phrase = words;
  wally_free_string(words);

  return pass_phrase;
}

void Wallet::RecoverWallet(
    const std::string& pass_phrase,
    ledger::ResultCallback callback) {
  recover_->Start(
      pass_phrase,
      [this, callback](const ledger::Result result) {
        if (result == ledger::Result::LEDGER_OK) {
          ledger_->database()->DeleteAllBalanceReports(
              [](const ledger::Result _) {});
          DisconnectAllWallets(callback);
          return;
        }
        callback(result);
      });
}

void Wallet::FetchBalance(ledger::FetchBalanceCallback callback) {
  balance_->Fetch(callback);
}

void Wallet::GetExternalWallet(
    const std::string& wallet_type,
    ledger::ExternalWalletCallback callback) {
  if (wallet_type == ledger::kWalletUphold) {
    uphold_->GenerateExternalWallet(
        [this, callback, wallet_type](const ledger::Result result) {
          if (result != ledger::Result::LEDGER_OK &&
              result != ledger::Result::CONTINUE) {
            callback(result, nullptr);
            return;
          }

          auto wallets = ledger_->ledger_client()->GetExternalWallets();
          auto wallet = GetWallet(wallet_type, std::move(wallets));
          callback(ledger::Result::LEDGER_OK, std::move(wallet));
        });
    return;
  }

  NOTREACHED();
  callback(ledger::Result::LEDGER_ERROR, nullptr);
}

void Wallet::ExternalWalletAuthorization(
    const std::string& wallet_type,
    const std::map<std::string, std::string>& args,
    ledger::ExternalWalletAuthorizationCallback callback) {
  auto wallets = ledger_->ledger_client()->GetExternalWallets();

  if (wallets.empty()) {
    BLOG(0, "No wallets");
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  if (wallet_type == ledger::kWalletUphold) {
    uphold_->WalletAuthorization(args, callback);
    return;
  }

  NOTREACHED();
  callback(ledger::Result::LEDGER_ERROR, {});
}

void Wallet::DisconnectWallet(
      const std::string& wallet_type,
      ledger::ResultCallback callback) {
  auto wallets = ledger_->ledger_client()->GetExternalWallets();

  if (wallets.empty()) {
    BLOG(0, "No wallets");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto wallet_ptr = GetWallet(wallet_type, std::move(wallets));

  if (!wallet_ptr) {
    BLOG(0, "Wallet is null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  wallet_ptr = ResetWallet(std::move(wallet_ptr));
  ledger_->ledger_client()->SaveExternalWallet(
      wallet_type,
      std::move(wallet_ptr));
  callback(ledger::Result::LEDGER_OK);
}

void Wallet::ClaimFunds(ledger::ResultCallback callback) {
  // Anonymous funds claim
  claim_->Start([this, callback](const ledger::Result result) {
    if (result != ledger::Result::LEDGER_OK &&
        result != ledger::Result::ALREADY_EXISTS) {
      BLOG(0, "Claiming anon funds failed");
      callback(ledger::Result::CONTINUE);
      return;
    }

    // tokens claim
    ledger_->promotion()->TransferTokens(
        [callback](const ledger::Result result) {
          if (result != ledger::Result::LEDGER_OK) {
            BLOG(0, "Claiming tokens failed");
            callback(ledger::Result::CONTINUE);
            return;
          }

          callback(ledger::Result::LEDGER_OK);
        });
  });
}

void Wallet::GetAnonWalletStatus(ledger::ResultCallback callback) {
  const std::string payment_id = ledger_->state()->GetPaymentId();
  const std::string passphrase = GetWalletPassphrase();
  const uint64_t stamp = ledger_->state()->GetCreationStamp();

  if (!payment_id.empty() && stamp != 0) {
    callback(ledger::Result::WALLET_CREATED);
    return;
  }

  if (payment_id.empty() || passphrase.empty()) {
    BLOG(0, "Wallet is corrupted");
    callback(ledger::Result::CORRUPTED_DATA);
    return;
  }

  callback(ledger::Result::LEDGER_OK);
}

void Wallet::DisconnectAllWallets(ledger::ResultCallback callback) {
  auto wallets = ledger_->ledger_client()->GetExternalWallets();

  if (wallets.empty()) {
    BLOG(1, "No wallets");
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  for (auto& wallet : wallets) {
    auto wallet_new = ResetWallet(std::move(wallet.second));
    if (!wallet_new) {
      continue;
    }

    ledger_->ledger_client()->SaveExternalWallet(
        wallet.first,
        std::move(wallet_new));
  }

  callback(ledger::Result::LEDGER_OK);
}

}  // namespace braveledger_wallet
