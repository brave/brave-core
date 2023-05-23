/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet_provider/uphold/connect_uphold_wallet.h"

#include <utility>

#include "base/containers/contains.h"
#include "base/task/single_thread_task_runner.h"
#include "brave/components/brave_rewards/core/common/random_util.h"
#include "brave/components/brave_rewards/core/endpoints/post_connect/uphold/post_connect_uphold.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/logging/event_log_keys.h"
#include "brave/components/brave_rewards/core/logging/logging.h"
#include "brave/components/brave_rewards/core/notifications/notification_keys.h"
#include "brave/components/brave_rewards/core/uphold/uphold.h"
#include "brave/components/brave_rewards/core/uphold/uphold_util.h"

namespace brave_rewards::internal {

using endpoints::PostConnectUphold;
using endpoints::PostOAuthUphold;
using endpoints::RequestFor;

namespace uphold {

ConnectUpholdWallet::ConnectUpholdWallet() {
  // Call StartEligibilityChecker() in a separate task to avoid cyclic
  // initialization (LedgerImpl => Uphold => ConnectUpholdWallet => LedgerImpl).
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&ConnectUpholdWallet::StartEligibilityChecker,
                                weak_factory_.GetWeakPtr()));
}

ConnectUpholdWallet::~ConnectUpholdWallet() = default;

void ConnectUpholdWallet::StartEligibilityChecker() {
  eligibility_checker_.Start(FROM_HERE,
                             base::Minutes(ledger().GetTesting() ? 3 : 15),
                             this, &ConnectUpholdWallet::CheckEligibility);
}

const char* ConnectUpholdWallet::WalletType() const {
  return constant::kWalletUphold;
}

void ConnectUpholdWallet::Authorize(OAuthInfo&& oauth_info,
                                    ConnectExternalWalletCallback callback) {
  DCHECK(!oauth_info.code.empty());

  RequestFor<PostOAuthUphold>(std::move(oauth_info.code))
      .Send(base::BindOnce(&ConnectUpholdWallet::OnAuthorize,
                           base::Unretained(this), std::move(callback)));
}

void ConnectUpholdWallet::OnAuthorize(ConnectExternalWalletCallback callback,
                                      PostOAuthUphold::Result&& result) {
  if (!ledger().uphold()->GetWalletIf({mojom::WalletStatus::kNotConnected,
                                       mojom::WalletStatus::kLoggedOut})) {
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (!result.has_value()) {
    BLOG(0, "Couldn't exchange code for the access token!");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  const std::string access_token = std::move(result.value());

  ledger().uphold()->GetUser(
      access_token,
      // NOLINTNEXTLINE
      base::BindOnce(
          static_cast<void (ConnectUpholdWallet::*)(
              ConnectExternalWalletCallback, const std::string&, mojom::Result,
              const User&) const>(&ConnectUpholdWallet::OnGetUser),
          base::Unretained(this), std::move(callback), access_token));
}

void ConnectUpholdWallet::OnGetUser(ConnectExternalWalletCallback callback,
                                    const std::string& access_token,
                                    mojom::Result result,
                                    const User& user) const {
  auto wallet = ledger().uphold()->GetWalletIf(
      {mojom::WalletStatus::kNotConnected, mojom::WalletStatus::kLoggedOut});
  if (!wallet) {
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (result == mojom::Result::EXPIRED_TOKEN) {
    BLOG(0, "Access token expired!");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Couldn't get user object from " << constant::kWalletUphold << '!');
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (user.bat_not_allowed) {
    BLOG(0, "BAT is not allowed for the user!");

    return std::move(callback).Run(base::unexpected(
        mojom::ConnectExternalWalletError::kUpholdBATNotAllowed));
  }

  wallet->user_name = user.name;
  wallet->member_id = user.member_id;
  if (!ledger().uphold()->SetWallet(std::move(wallet))) {
    BLOG(0, "Failed to save " << constant::kWalletUphold << " wallet!");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  ledger().uphold()->GetCapabilities(
      access_token,
      base::BindOnce(
          // NOLINTNEXTLINE
          static_cast<void (ConnectUpholdWallet::*)(
              ConnectExternalWalletCallback, const std::string&, mojom::Result,
              internal::uphold::Capabilities) const>(
              &ConnectUpholdWallet::OnGetCapabilities),
          base::Unretained(this), std::move(callback), access_token));
}

void ConnectUpholdWallet::OnGetCapabilities(
    ConnectExternalWalletCallback callback,
    const std::string& access_token,
    mojom::Result result,
    Capabilities capabilities) const {
  auto wallet = ledger().uphold()->GetWalletIf(
      {mojom::WalletStatus::kNotConnected, mojom::WalletStatus::kLoggedOut});
  if (!wallet) {
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (result == mojom::Result::EXPIRED_TOKEN) {
    BLOG(0, "Access token expired!");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (result != mojom::Result::LEDGER_OK || !capabilities.can_receive ||
      !capabilities.can_send) {
    BLOG(0,
         "Couldn't get capabilities from " << constant::kWalletUphold << '!');
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (!*capabilities.can_receive || !*capabilities.can_send) {
    BLOG(0, "User doesn't have the required " << constant::kWalletUphold
                                              << " capabilities!");

    return std::move(callback).Run(base::unexpected(
        mojom::ConnectExternalWalletError::kUpholdInsufficientCapabilities));
  }

  ledger().uphold()->CreateCard(
      access_token,
      base::BindOnce(&ConnectUpholdWallet::OnCreateCard, base::Unretained(this),
                     std::move(callback), access_token));
}

void ConnectUpholdWallet::OnCreateCard(ConnectExternalWalletCallback callback,
                                       const std::string& access_token,
                                       mojom::Result result,
                                       std::string&& id) const {
  if (!ledger().uphold()->GetWalletIf({mojom::WalletStatus::kNotConnected,
                                       mojom::WalletStatus::kLoggedOut})) {
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (result == mojom::Result::EXPIRED_TOKEN) {
    BLOG(0, "Access token expired!");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (result != mojom::Result::LEDGER_OK) {
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (id.empty()) {
    BLOG(0, "Card ID is empty!");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  auto on_connect =
      base::BindOnce(&ConnectUpholdWallet::OnConnect, base::Unretained(this),
                     std::move(callback), access_token, id);

  RequestFor<PostConnectUphold>(std::move(id)).Send(std::move(on_connect));
}

void ConnectUpholdWallet::CheckEligibility() {
  auto wallet =
      ledger().uphold()->GetWalletIf({mojom::WalletStatus::kConnected});
  if (!wallet) {
    return;
  }

  ledger().uphold()->GetUser(
      // NOLINTNEXTLINE
      wallet->token, base::BindOnce(static_cast<void (ConnectUpholdWallet::*)(
                                        mojom::Result, const User&) const>(
                                        &ConnectUpholdWallet::OnGetUser),
                                    base::Unretained(this)));
}

void ConnectUpholdWallet::OnGetUser(mojom::Result result,
                                    const User& user) const {
  auto wallet =
      ledger().uphold()->GetWalletIf({mojom::WalletStatus::kConnected});
  if (!wallet) {
    return;
  }

  if (result == mojom::Result::EXPIRED_TOKEN) {
    BLOG(0, "Access token expired!");
    // kConnected ==> kLoggedOut
    if (!ledger().uphold()->LogOutWallet()) {
      BLOG(0, "Failed to disconnect " << constant::kWalletUphold << " wallet!");
    }

    return;
  }

  if (result != mojom::Result::LEDGER_OK) {
    return BLOG(
        0, "Couldn't get user object from " << constant::kWalletUphold << '!');
  }

  if (user.bat_not_allowed) {
    BLOG(0, "BAT is not allowed for the user!");

    // kConnected ==> kLoggedOut
    if (!ledger().uphold()->LogOutWallet(notifications::kUpholdBATNotAllowed)) {
      BLOG(0, "Failed to disconnect " << constant::kWalletUphold << " wallet!");
    }

    return;
  }

  ledger().uphold()->GetCapabilities(
      wallet->token,
      base::BindOnce(
          // NOLINTNEXTLINE
          static_cast<void (ConnectUpholdWallet::*)(mojom::Result, Capabilities)
                          const>(&ConnectUpholdWallet::OnGetCapabilities),
          base::Unretained(this)));
}

void ConnectUpholdWallet::OnGetCapabilities(mojom::Result result,
                                            Capabilities capabilities) const {
  if (!ledger().uphold()->GetWalletIf({mojom::WalletStatus::kConnected})) {
    return;
  }

  if (result == mojom::Result::EXPIRED_TOKEN) {
    BLOG(0, "Access token expired!");
    // kConnected ==> kLoggedOut
    if (!ledger().uphold()->LogOutWallet()) {
      BLOG(0, "Failed to disconnect " << constant::kWalletUphold << " wallet!");
    }

    return;
  }

  if (result != mojom::Result::LEDGER_OK || !capabilities.can_receive ||
      !capabilities.can_send) {
    return BLOG(
        0, "Couldn't get capabilities from " << constant::kWalletUphold << '!');
  }

  if (!*capabilities.can_receive || !*capabilities.can_send) {
    BLOG(0, "User doesn't have the required " << constant::kWalletUphold
                                              << " capabilities!");

    // kConnected ==> kLoggedOut
    if (!ledger().uphold()->LogOutWallet(
            notifications::kUpholdInsufficientCapabilities)) {
      BLOG(0, "Failed to disconnect " << constant::kWalletUphold << " wallet!");
    }
  }
}

}  // namespace uphold

}  // namespace brave_rewards::internal
