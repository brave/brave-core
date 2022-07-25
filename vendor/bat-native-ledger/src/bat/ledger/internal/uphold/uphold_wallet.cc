/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/uphold/uphold_wallet.h"

#include <memory>
#include <utility>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/common/random_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"
#include "bat/ledger/internal/logging/event_log_util.h"
#include "bat/ledger/internal/notifications/notification_keys.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "bat/ledger/internal/wallet/wallet_util.h"

using ledger::uphold::Capabilities;
using ledger::wallet::OnWalletStatusChange;

namespace ledger {
namespace uphold {

UpholdWallet::UpholdWallet(LedgerImpl* ledger)
    : ledger_{ledger},
      promotion_server_{std::make_unique<endpoint::PromotionServer>(ledger)} {}

UpholdWallet::~UpholdWallet() = default;

void UpholdWallet::Generate(ledger::ResultCallback callback) const {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    uphold_wallet = type::ExternalWallet::New();
    uphold_wallet->type = constant::kWalletUphold;
    uphold_wallet->status = type::WalletStatus::NOT_CONNECTED;
    if (!ledger_->uphold()->SetWallet(uphold_wallet->Clone())) {
      BLOG(0, "Unable to set the Uphold wallet!");
      return std::move(callback).Run(type::Result::LEDGER_ERROR);
    }

    OnWalletStatusChange(ledger_, {}, uphold_wallet->status);
  }

  if (uphold_wallet->one_time_string.empty()) {
    uphold_wallet->one_time_string = util::GenerateRandomHexString();
  }
  uphold_wallet = GenerateLinks(std::move(uphold_wallet));

  const auto status = uphold_wallet->status;
  if (!ledger_->uphold()->SetWallet(std::move(uphold_wallet))) {
    BLOG(0, "Unable to set the Uphold wallet!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR);
  }

  // In VERIFIED state TransferTokens is done in OnGetUser after checking
  // certain user properties. In DISCONNECTED_VERIFIED state we do not call
  // GetUser (because the token and address are empty, so the user cannot be
  // retrieved), so call TransferTokens here instead.
  if (status == type::WalletStatus::DISCONNECTED_VERIFIED) {
    return ledger_->promotion()->TransferTokens(
        base::BindOnce(&UpholdWallet::OnTransferTokens, base::Unretained(this),
                       std::move(callback)));
  }

  if (status != type::WalletStatus::PENDING &&
      status != type::WalletStatus::VERIFIED) {
    return std::move(callback).Run(type::Result::LEDGER_OK);
  }

  ledger_->uphold()->GetUser(base::BindOnce(
      &UpholdWallet::OnGetUser, base::Unretained(this), std::move(callback)));
}

void UpholdWallet::OnGetUser(ledger::ResultCallback callback,
                             type::Result result,
                             const User& user) const {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "Uphold wallet is null!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR);
  }

  if (uphold_wallet->status != type::WalletStatus::PENDING &&
      uphold_wallet->status != type::WalletStatus::VERIFIED) {
    return std::move(callback).Run(type::Result::LEDGER_OK);
  }

  CheckWalletState(uphold_wallet.get());

  if (result == type::Result::EXPIRED_TOKEN) {
    BLOG(0, "Access token expired!");
    // Entering NOT_CONNECTED or DISCONNECTED_VERIFIED.
    ledger_->uphold()->DisconnectWallet(
        ledger::notifications::kWalletDisconnected);
    return std::move(callback).Run(type::Result::EXPIRED_TOKEN);
  }

  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Couldn't get the user object from Uphold!");
    return std::move(callback).Run(type::Result::CONTINUE);
  }

  if (user.bat_not_allowed) {
    BLOG(0, "BAT is not allowed for the user!");
    // Entering NOT_CONNECTED or DISCONNECTED_VERIFIED.
    ledger_->uphold()->DisconnectWallet("");
    return std::move(callback).Run(type::Result::UPHOLD_BAT_NOT_ALLOWED);
  }

  uphold_wallet->user_name = user.name;
  uphold_wallet->member_id = user.member_id;
  if (!ledger_->uphold()->SetWallet(uphold_wallet->Clone())) {
    BLOG(0, "Unable to set the Uphold wallet!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR);
  }

  ledger_->uphold()->GetCapabilities(
      base::BindOnce(&UpholdWallet::OnGetCapabilities, base::Unretained(this),
                     std::move(callback)));
}

void UpholdWallet::OnGetCapabilities(ledger::ResultCallback callback,
                                     type::Result result,
                                     Capabilities capabilities) const {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "Uphold wallet is null!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR);
  }

  if (uphold_wallet->status != type::WalletStatus::PENDING &&
      uphold_wallet->status != type::WalletStatus::VERIFIED) {
    return std::move(callback).Run(type::Result::LEDGER_OK);
  }

  CheckWalletState(uphold_wallet.get());

  if (result == type::Result::EXPIRED_TOKEN) {
    BLOG(0, "Access token expired!");
    // Entering NOT_CONNECTED or DISCONNECTED_VERIFIED.
    ledger_->uphold()->DisconnectWallet(
        ledger::notifications::kWalletDisconnected);
    return std::move(callback).Run(type::Result::EXPIRED_TOKEN);
  }

  if (result != type::Result::LEDGER_OK || !capabilities.can_receive ||
      !capabilities.can_send) {
    BLOG(0, "Couldn't get capabilities from Uphold!");
    return std::move(callback).Run(type::Result::CONTINUE);
  }

  if (!*capabilities.can_receive || !*capabilities.can_send) {
    BLOG(0, "User doesn't have the required Uphold capabilities!");
    // Entering NOT_CONNECTED.
    if (uphold_wallet->status == type::WalletStatus::VERIFIED) {
      ledger_->ledger_client()->ShowNotification(
          ledger::notifications::kWalletDisconnected, {"Uphold"}, [](auto) {});
      ledger_->wallet()->DisconnectWallet(constant::kWalletUphold, [](auto) {});
    } else {
      ledger_->uphold()->DisconnectWallet("");
    }
    return std::move(callback).Run(
        type::Result::UPHOLD_INSUFFICIENT_CAPABILITIES);
  }

  if (uphold_wallet->status == type::WalletStatus::VERIFIED) {
    return ledger_->promotion()->TransferTokens(
        base::BindOnce(&UpholdWallet::OnTransferTokens, base::Unretained(this),
                       std::move(callback)));
  }

  ledger_->uphold()->CreateCard(base::BindOnce(&UpholdWallet::OnCreateCard,
                                               base::Unretained(this),
                                               std::move(callback)));
}

void UpholdWallet::OnCreateCard(ledger::ResultCallback callback,
                                type::Result result,
                                const std::string& id) const {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "Uphold wallet is null!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR);
  }

  if (uphold_wallet->status != type::WalletStatus::PENDING) {
    return std::move(callback).Run(type::Result::LEDGER_OK);
  }

  CheckWalletState(uphold_wallet.get());

  if (result == type::Result::EXPIRED_TOKEN) {
    BLOG(0, "Access token expired!");
    // Entering NOT_CONNECTED.
    ledger_->uphold()->DisconnectWallet(
        ledger::notifications::kWalletDisconnected);
    return std::move(callback).Run(type::Result::EXPIRED_TOKEN);
  }

  if (result != type::Result::LEDGER_OK) {
    return std::move(callback).Run(type::Result::CONTINUE);
  }

  if (id.empty()) {
    BLOG(0, "Card ID is empty!");
    return std::move(callback).Run(type::Result::CONTINUE);
  }

  GetAnonFunds(base::BindOnce(&UpholdWallet::OnGetAnonFunds,
                              base::Unretained(this), id, std::move(callback)));
}

void UpholdWallet::GetAnonFunds(
    endpoint::promotion::GetWalletBalanceCallback callback) const {
  // if we don't have user funds in anon card anymore
  // we can skip balance server ping
  if (!ledger_->state()->GetFetchOldBalanceEnabled()) {
    return std::move(callback).Run(type::Result::LEDGER_OK,
                                   type::Balance::New());
  }

  const auto rewards_wallet = ledger_->wallet()->GetWallet();
  if (!rewards_wallet) {
    BLOG(1, "Rewards wallet is null!");
    ledger_->state()->SetFetchOldBalanceEnabled(false);
    return std::move(callback).Run(type::Result::LEDGER_OK,
                                   type::Balance::New());
  }

  if (rewards_wallet->payment_id.empty()) {
    BLOG(0, "Payment ID is empty!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR, nullptr);
  }

  promotion_server_->get_wallet_balance()->Request(std::move(callback));
}

void UpholdWallet::OnGetAnonFunds(const std::string& id,
                                  ledger::ResultCallback callback,
                                  const type::Result result,
                                  type::BalancePtr balance) const {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "Uphold wallet is null!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR);
  }

  if (uphold_wallet->status != type::WalletStatus::PENDING) {
    return std::move(callback).Run(type::Result::LEDGER_OK);
  }

  CheckWalletState(uphold_wallet.get());
  DCHECK(!id.empty());

  if (result != type::Result::LEDGER_OK || !balance) {
    BLOG(0, "Couldn't get anonymous funds!");
    return std::move(callback).Run(type::Result::CONTINUE);
  }

  if (balance->user_funds == 0.0) {  // == floating-point comparison!
    ledger_->state()->SetFetchOldBalanceEnabled(false);
  }

  LinkWallet(balance->user_funds, id,
             base::BindOnce(&UpholdWallet::OnLinkWallet, base::Unretained(this),
                            std::move(callback)));
}

void UpholdWallet::LinkWallet(
    double user_funds,
    const std::string& id,
    ledger::endpoint::promotion::PostClaimUpholdCallback callback) const {
  promotion_server_->post_claim_uphold()->Request(user_funds, id,
                                                  std::move(callback));
}

void UpholdWallet::OnLinkWallet(ledger::ResultCallback callback,
                                type::Result result,
                                const std::string& id) const {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "Uphold wallet is null!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR);
  }

  if (uphold_wallet->status != type::WalletStatus::PENDING) {
    return std::move(callback).Run(type::Result::LEDGER_OK);
  }

  CheckWalletState(uphold_wallet.get());
  DCHECK(!id.empty());

  switch (result) {
    case type::Result::DEVICE_LIMIT_REACHED:
    case type::Result::MISMATCHED_PROVIDER_ACCOUNTS:
    case type::Result::NOT_FOUND:  // KYC required
    case type::Result::UPHOLD_TRANSACTION_VERIFICATION_FAILURE:
    case type::Result::FLAGGED_WALLET:
    case type::Result::REGION_NOT_SUPPORTED:
      // Entering NOT_CONNECTED.
      ledger_->uphold()->DisconnectWallet("");
      ledger_->database()->SaveEventLog(
          log::GetEventLogKeyForLinkingResult(result),
          constant::kWalletUphold + std::string("/") + id.substr(0, 5));
      return std::move(callback).Run(result);
    default:
      if (result != type::Result::LEDGER_OK) {
        BLOG(0, "Couldn't claim wallet!");
        return std::move(callback).Run(type::Result::CONTINUE);
      }
  }

  const auto from = uphold_wallet->status;
  const auto to = uphold_wallet->status = type::WalletStatus::VERIFIED;
  uphold_wallet->address = id;
  uphold_wallet = GenerateLinks(std::move(uphold_wallet));
  if (!ledger_->uphold()->SetWallet(std::move(uphold_wallet))) {
    BLOG(0, "Unable to set the Uphold wallet!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR);
  }

  OnWalletStatusChange(ledger_, from, to);

  ledger_->database()->SaveEventLog(
      log::kWalletVerified,
      constant::kWalletUphold + std::string("/") + id.substr(0, 5));

  ledger_->promotion()->TransferTokens(
      base::BindOnce(&UpholdWallet::OnTransferTokens, base::Unretained(this),
                     std::move(callback)));
}

void UpholdWallet::OnTransferTokens(ledger::ResultCallback callback,
                                    type::Result result,
                                    std::string) const {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "Uphold wallet is null!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR);
  }

  if (uphold_wallet->status != type::WalletStatus::VERIFIED &&
      uphold_wallet->status != type::WalletStatus::DISCONNECTED_VERIFIED) {
    return std::move(callback).Run(type::Result::LEDGER_OK);
  }

  CheckWalletState(uphold_wallet.get());

  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Transferring tokens failed!");
    return std::move(callback).Run(type::Result::CONTINUE);
  }

  std::move(callback).Run(type::Result::LEDGER_OK);
}

}  // namespace uphold
}  // namespace ledger
