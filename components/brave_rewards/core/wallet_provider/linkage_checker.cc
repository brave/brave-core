/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet_provider/linkage_checker.h"

#include <string>
#include <utility>

#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/notifications/notification_keys.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/wallet/wallet_util.h"

namespace brave_rewards::internal {

LinkageChecker::LinkageChecker(RewardsEngineImpl& engine)
    : RewardsEngineHelper(engine) {}

LinkageChecker::~LinkageChecker() = default;

void LinkageChecker::Start() {
  if (timer_.IsRunning()) {
    return;
  }
  CheckLinkage();
  timer_.Start(FROM_HERE, base::Hours(24), this, &LinkageChecker::CheckLinkage);
}

void LinkageChecker::Stop() {
  timer_.Stop();
}

void LinkageChecker::CheckLinkage() {
  if (!ShouldPerformCheck()) {
    return;
  }
  endpoints::RequestFor<endpoints::GetWallet>(engine()).Send(base::BindOnce(
      &LinkageChecker::CheckLinkageCallback, weak_factory_.GetWeakPtr()));
}

bool LinkageChecker::ShouldPerformCheck() {
  auto wallet = engine().wallet()->GetWallet();
  if (!wallet || wallet->payment_id.empty()) {
    return false;
  }
  return !GetExternalWallet().is_null();
}

mojom::ExternalWalletPtr LinkageChecker::GetExternalWallet() {
  auto wallet_type = engine().GetState<std::string>(state::kExternalWalletType);
  if (wallet_type.empty()) {
    return nullptr;
  }
  return wallet::GetWalletIf(
      engine(), wallet_type,
      {mojom::WalletStatus::kConnected, mojom::WalletStatus::kLoggedOut});
}

void LinkageChecker::MaybeUpdateExternalWalletStatus(
    endpoints::GetWallet::Value& value) {
  auto& [wallet_provider, linked] = value;
  auto wallet = GetExternalWallet();

  // If the user has a connected wallet, but the server indicates that the user
  // is no longer linked to that provider, then transition the user back into
  // the not-connected state. Note that this does not handle the situation where
  // the server indicates that the user is connected to a different wallet
  // provider.
  if (wallet && wallet_provider == wallet->type && !linked) {
    // {kConnected, kLoggedOut} ==> kNotConnected
    if (wallet::TransitionWallet(engine(), std::move(wallet),
                                 mojom::WalletStatus::kNotConnected)) {
      client().ExternalWalletDisconnected();
    } else {
      LogError(FROM_HERE) << "Failed to transition " << wallet->type
                          << " wallet state";
    }
  }
}

void LinkageChecker::CheckLinkageCallback(
    endpoints::GetWallet::Result&& result) {
  if (!result.has_value()) {
    return;
  }
  MaybeUpdateExternalWalletStatus(result.value());
}

}  // namespace brave_rewards::internal
