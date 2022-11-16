/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet_provider/bitflyer/connect_bitflyer_wallet.h"

#include <utility>

#include "base/containers/contains.h"
#include "base/strings/string_number_conversions.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/bitflyer/bitflyer.h"
#include "bat/ledger/internal/bitflyer/bitflyer_util.h"
#include "bat/ledger/internal/common/random_util.h"
#include "bat/ledger/internal/endpoint/bitflyer/bitflyer_server.h"
#include "bat/ledger/internal/endpoints/post_connect/bitflyer/post_connect_bitflyer.h"
#include "bat/ledger/internal/endpoints/request_for.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"
#include "crypto/sha2.h"

using ledger::endpoints::PostConnectBitflyer;
using ledger::endpoints::RequestFor;
using ledger::wallet_provider::ConnectExternalWallet;

namespace ledger::bitflyer {

ConnectBitFlyerWallet::ConnectBitFlyerWallet(LedgerImpl* ledger)
    : ConnectExternalWallet(ledger),
      bitflyer_server_(std::make_unique<endpoint::BitflyerServer>(ledger)) {}

ConnectBitFlyerWallet::~ConnectBitFlyerWallet() = default;

const char* ConnectBitFlyerWallet::WalletType() const {
  return constant::kWalletBitflyer;
}

void ConnectBitFlyerWallet::Authorize(
    OAuthInfo&& oauth_info,
    ledger::ConnectExternalWalletCallback callback) const {
  DCHECK(!oauth_info.code.empty());
  DCHECK(!oauth_info.code_verifier.empty());

  const auto rewards_wallet = ledger_->wallet()->GetWallet();
  if (!rewards_wallet) {
    BLOG(0, "Rewards wallet is null!");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  const std::string hashed_payment_id =
      crypto::SHA256HashString(rewards_wallet->payment_id);
  const std::string external_account_id =
      base::HexEncode(hashed_payment_id.data(), hashed_payment_id.size());

  bitflyer_server_->post_oauth()->Request(
      external_account_id, std::move(oauth_info.code),
      std::move(oauth_info.code_verifier),
      base::BindOnce(&ConnectBitFlyerWallet::OnAuthorize,
                     base::Unretained(this), std::move(callback)));
}

void ConnectBitFlyerWallet::OnAuthorize(
    ledger::ConnectExternalWalletCallback callback,
    mojom::Result result,
    std::string&& token,
    std::string&& address,
    std::string&& linking_info) const {
  if (!ledger_->bitflyer()->GetWalletIf({mojom::WalletStatus::kNotConnected,
                                         mojom::WalletStatus::kLoggedOut})) {
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Couldn't get token");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (token.empty()) {
    BLOG(0, "Token is empty");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (address.empty()) {
    BLOG(0, "Address is empty");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (linking_info.empty()) {
    BLOG(0, "Linking info is empty");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  auto on_connect =
      base::BindOnce(&ConnectBitFlyerWallet::OnConnect, base::Unretained(this),
                     std::move(callback), std::move(token), std::move(address));

  RequestFor<PostConnectBitflyer>(ledger_, std::move(linking_info))
      .Send(std::move(on_connect));
}

}  // namespace ledger::bitflyer
