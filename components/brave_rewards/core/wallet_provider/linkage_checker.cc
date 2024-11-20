/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet_provider/linkage_checker.h"

#include <string>
#include <utility>

#include "base/feature_list.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/notifications/notification_keys.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "brave/components/brave_rewards/core/wallet/wallet_util.h"
#include "brave/components/brave_rewards/core/wallet_provider/wallet_provider.h"

namespace brave_rewards::internal {

LinkageChecker::LinkageChecker(RewardsEngine& engine)
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
  if (check_in_progress_ || !ShouldPerformCheck()) {
    return;
  }
  check_in_progress_ = true;
  endpoints::RequestFor<endpoints::GetWallet>(engine()).Send(base::BindOnce(
      &LinkageChecker::CheckLinkageCallback, weak_factory_.GetWeakPtr()));
}

bool LinkageChecker::ShouldPerformCheck() {
  auto wallet = engine().wallet()->GetWallet();
  if (!wallet || wallet->payment_id.empty()) {
    return false;
  }
  return true;
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
  auto wallet = GetExternalWallet();

  // If the user has a connected wallet, but the server indicates that the user
  // is no longer linked to that provider, then transition the user back into
  // the not-connected state. Note that this does not handle the situation where
  // the server indicates that the user is connected to a different wallet
  // provider.
  if (wallet && value.wallet_provider == wallet->type && !value.linked) {
    // {kConnected, kLoggedOut} ==> kNotConnected
    if (wallet::TransitionWallet(engine(), std::move(wallet),
                                 mojom::WalletStatus::kNotConnected)) {
      client().ExternalWalletDisconnected();
      wallet = nullptr;
    } else {
      LogError(FROM_HERE) << "Failed to transition wallet state";
      return;
    }
  }

  // If the user is not connected and the server indicates that the account is
  // linked to an external wallet provider, then notify the external wallet
  // provider handler. Some providers may be able to complete the linkage at
  // this point.
  if (!wallet && value.linked) {
    auto* provider = engine().GetExternalWalletProvider(value.wallet_provider);
    if (!provider) {
      LogError(FROM_HERE) << "Invalid external wallet type";
      return;
    }
    DCHECK(!value.provider_id.empty());
    provider->OnWalletLinked(value.provider_id);
  }
}

void LinkageChecker::UpdateSelfCustodyAvailableDict(
    endpoints::GetWallet::Value& value) {
  if (!base::FeatureList::IsEnabled(
          features::kAllowSelfCustodyProvidersFeature)) {
    return;
  }

  // Returns true if the supplied Dict has a true value for some supported
  // wallet provider key.
  auto has_available_provider = [&](const base::Value::Dict* dict) {
    if (!dict) {
      return false;
    }
    for (auto&& [key, entry_value] : *dict) {
      if (auto bool_value = entry_value.GetIfBool()) {
        if (*bool_value && engine().GetExternalWalletProvider(key)) {
          return true;
        }
      }
    }
    return false;
  };

  bool previously_available = has_available_provider(
      engine().GetState<base::Value>(state::kSelfCustodyAvailable).GetIfDict());

  bool currently_available =
      has_available_provider(&value.self_custody_available);

  // Save the available self-custody providers for this user.
  engine().SetState(state::kSelfCustodyAvailable,
                    base::Value(std::move(value.self_custody_available)));

  // If the user is transitioning from having no self-custody providers
  // available to having at least one supported provider available, and they
  // have not dismissed the invite, then notify the user that self-custody
  // providers are available.
  if (!previously_available && currently_available) {
    if (!engine().GetState<bool>(state::kSelfCustodyInviteDismissed)) {
      engine().client()->ShowNotification(notifications::kSelfCustodyAvailable,
                                          {}, base::DoNothing());
    }
  }
}

void LinkageChecker::CheckLinkageCallback(
    endpoints::GetWallet::Result&& result) {
  check_in_progress_ = false;
  if (!result.has_value()) {
    return;
  }
  MaybeUpdateExternalWalletStatus(result.value());
  UpdateSelfCustodyAvailableDict(result.value());
}

}  // namespace brave_rewards::internal
