/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/guid.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/endpoint/gemini/gemini_server.h"
#include "bat/ledger/internal/gemini/gemini.h"
#include "bat/ledger/internal/gemini/gemini_authorization.h"
#include "bat/ledger/internal/gemini/gemini_transfer.h"
#include "bat/ledger/internal/gemini/gemini_util.h"
#include "bat/ledger/internal/gemini/gemini_wallet.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/wallet/wallet_util.h"
#include "brave_base/random.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace {
const char kFeeMessage[] =
    "5% transaction fee collected by Brave Software International";
const double kTransactionFee = 0.05;
}  // namespace

namespace ledger {
namespace gemini {

Gemini::Gemini(LedgerImpl* ledger)
    : transfer_(std::make_unique<GeminiTransfer>(ledger)),
      authorization_(std::make_unique<GeminiAuthorization>(ledger)),
      wallet_(std::make_unique<GeminiWallet>(ledger)),
      gemini_server_(std::make_unique<endpoint::GeminiServer>(ledger)),
      ledger_(ledger) {}

Gemini::~Gemini() = default;

void Gemini::Initialize() {
  auto wallet = GetWallet();
  if (!wallet) {
    return;
  }

  for (const auto& value : wallet->fees) {
    StartTransferFeeTimer(value.first, 1);
  }
}

void Gemini::StartContribution(const std::string& contribution_id,
                               type::ServerPublisherInfoPtr info,
                               const double amount,
                               ResultCallback callback) {
  if (!info) {
    BLOG(0, "Publisher info is null");
    ContributionCompleted(type::Result::LEDGER_ERROR, "", contribution_id,
                          amount, "", callback);
    return;
  }

  const double fee = (amount * kTransactionFee);
  const double reconcile_amount = amount - fee;

  auto contribution_callback =
      std::bind(&Gemini::ContributionCompleted, this, _1, _2, contribution_id,
                fee, info->publisher_key, callback);

  Transaction transaction;
  transaction.address = info->address;
  transaction.amount = reconcile_amount;

  transfer_->Start(transaction, contribution_callback);
}

void Gemini::ContributionCompleted(const type::Result result,
                                   const std::string& transaction_id,
                                   const std::string& contribution_id,
                                   const double fee,
                                   const std::string& publisher_key,
                                   ResultCallback callback) {
  if (result == type::Result::LEDGER_OK) {
    SaveTransferFee(contribution_id, fee);

    if (!publisher_key.empty()) {
      ledger_->database()->UpdateContributionInfoContributedAmount(
          contribution_id, publisher_key, callback);
      return;
    }
  }

  callback(result);
}

void Gemini::FetchBalance(FetchBalanceCallback callback) {
  const auto wallet = GetWallet();

  if (!wallet || wallet->token.empty() || wallet->address.empty()) {
    callback(type::Result::LEDGER_OK, 0.0);
    return;
  }

  if (wallet->status != type::WalletStatus::VERIFIED) {
    BLOG(1, "Wallet is not verified");
    callback(type::Result::LEDGER_OK, 0.0);
    return;
  }

  auto url_callback =
      std::bind(&Gemini::OnFetchBalance, this, _1, _2, callback);

  gemini_server_->post_balance()->Request(wallet->token, url_callback);
}

void Gemini::OnFetchBalance(const type::Result result,
                            const double available,
                            FetchBalanceCallback callback) {
  if (result == type::Result::EXPIRED_TOKEN) {
    BLOG(0, "Expired token");
    DisconnectWallet();
    callback(type::Result::EXPIRED_TOKEN, 0.0);
    return;
  }

  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Couldn't get balance");
    callback(type::Result::LEDGER_ERROR, 0.0);
    return;
  }

  callback(type::Result::LEDGER_OK, available);
}

void Gemini::TransferFunds(const double amount,
                           const std::string& address,
                           client::TransactionCallback callback) {
  Transaction transaction;
  transaction.address = address;
  transaction.amount = amount;
  transfer_->Start(transaction, callback);
}

void Gemini::WalletAuthorization(
    const base::flat_map<std::string, std::string>& args,
    ExternalWalletAuthorizationCallback callback) {
  authorization_->Authorize(args, callback);
}

void Gemini::GenerateWallet(ResultCallback callback) {
  wallet_->Generate(callback);
}

void Gemini::DisconnectWallet(const bool manual) {
  auto wallet = GetWallet();
  if (!wallet) {
    return;
  }

  BLOG(1, "Disconnecting wallet");
  ledger_->database()->SaveEventLog(log::kWalletDisconnected,
                                    std::string(constant::kWalletGemini) +
                                        (!wallet->address.empty() ? "/" : "") +
                                        wallet->address.substr(0, 5));

  wallet = ::ledger::wallet::ResetWallet(std::move(wallet));
  if (manual) {
    wallet->status = type::WalletStatus::NOT_CONNECTED;
  }

  const bool shutting_down = ledger_->IsShuttingDown();

  if (!manual && !shutting_down) {
    ledger_->ledger_client()->ShowNotification("wallet_disconnected", {},
                                               [](type::Result _) {});
  }

  SetWallet(wallet->Clone());

  if (!shutting_down) {
    ledger_->ledger_client()->WalletDisconnected(constant::kWalletGemini);
  }
}

void Gemini::SaveTransferFee(const std::string& contribution_id,
                             const double fee) {
  StartTransferFeeTimer(contribution_id, 1);

  auto wallet = GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return;
  }

  wallet->fees.insert(std::make_pair(contribution_id, fee));
  SetWallet(std::move(wallet));
}

void Gemini::StartTransferFeeTimer(const std::string& fee_id,
                                   const int attempts) {
  DCHECK(!fee_id.empty());

  base::TimeDelta delay =
      util::GetRandomizedDelay(base::TimeDelta::FromSeconds(45));

  BLOG(1, "Gemini transfer fee timer set for " << delay);

  transfer_fee_timers_[fee_id].Start(
      FROM_HERE, delay,
      base::BindOnce(&Gemini::OnTransferFeeTimerElapsed, base::Unretained(this),
                     fee_id, attempts));
}

void Gemini::OnTransferFeeCompleted(const type::Result result,
                                    const std::string& transaction_id,
                                    const std::string& contribution_id,
                                    const int attempts) {
  if (result != type::Result::LEDGER_OK) {
    if (attempts < 3) {
      BLOG(0, "Transaction fee failed, retrying");
      StartTransferFeeTimer(contribution_id, attempts + 1);
      return;
    }
    BLOG(0, "Transaction fee failed, no remaining attempts this session");
    return;
  }

  RemoveTransferFee(contribution_id);
}

void Gemini::TransferFee(const std::string& contribution_id,
                         const double amount,
                         const int attempts) {
  auto transfer_callback = std::bind(&Gemini::OnTransferFeeCompleted, this, _1,
                                     _2, contribution_id, attempts);

  Transaction transaction;
  transaction.address = GetFeeAddress();
  transaction.amount = amount;
  transaction.message = kFeeMessage;

  transfer_->Start(transaction, transfer_callback);
}

void Gemini::OnTransferFeeTimerElapsed(const std::string& id,
                                       const int attempts) {
  transfer_fee_timers_.erase(id);

  auto wallet = GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return;
  }

  for (const auto& value : wallet->fees) {
    if (value.first == id) {
      TransferFee(value.first, value.second, attempts);
      return;
    }
  }
}

type::ExternalWalletPtr Gemini::GetWallet() {
  return ::ledger::wallet::GetWallet(ledger_, constant::kWalletGemini);
}

bool Gemini::SetWallet(type::ExternalWalletPtr wallet) {
  return ::ledger::wallet::SetWallet(ledger_, std::move(wallet),
                                     state::kWalletGemini);
}

void Gemini::RemoveTransferFee(const std::string& contribution_id) {
  auto wallet = GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return;
  }

  wallet->fees.erase(contribution_id);
  SetWallet(std::move(wallet));
}

}  // namespace gemini
}  // namespace ledger
