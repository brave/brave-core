/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/brave_account_service.h"

#include <utility>
#include <variant>

#include "absl/functional/overload.h"
#include "base/check.h"
#include "base/check_deref.h"
#include "base/check_is_test.h"
#include "base/functional/bind.h"
#include "base/notreached.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_account {

BraveAccountService::BraveAccountService(
    PrefService* pref_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    os_crypt_async::OSCryptAsync* os_crypt_async)
    : account_state_prefs_(CHECK_DEREF(pref_service)),
      url_loader_factory_(std::move(url_loader_factory)) {
  CHECK(url_loader_factory_);

  CHECK_DEREF(os_crypt_async)
      .GetInstance(base::BindOnce(&BraveAccountService::FinishInitialization,
                                  weak_factory_.GetWeakPtr()));
}

BraveAccountService::~BraveAccountService() = default;

void BraveAccountService::BindInterface(
    mojo::PendingReceiver<mojom::Authentication> pending_receiver) {
  const bool initializing = std::holds_alternative<std::monostate>(state_);
  initializing ? pending_receivers_.push_back(std::move(pending_receiver))
               : ActiveState().AddReceiver(std::move(pending_receiver));
}

base::OneShotTimer* BraveAccountService::AuthValidateTimerForTesting() {
  CHECK_IS_TEST();
  auto* logged_in = std::get_if<LoggedInState>(&state_);
  return logged_in ? &logged_in->auth_validate_timer_for_testing()  // IN-TEST
                   : nullptr;
}

void BraveAccountService::FinishInitialization(
    scoped_refptr<os_crypt_async::Encryptor> encryptor) {
  encryptor_ = std::move(encryptor);

  EnsureState(account_state_prefs_.GetAccountState()->which());

  account_state_prefs_.StartObserving(base::BindRepeating(
      &BraveAccountService::OnAccountStateChanged, base::Unretained(this)));
}

void BraveAccountService::AddObserver(
    mojo::PendingRemote<mojom::AuthenticationObserver> observer) {
  const auto observer_id = observers_.Add(std::move(observer));
  CHECK_DEREF(observers_.Get(observer_id))
      .OnAccountStateChanged(account_state_prefs_.GetAccountState());
}

void BraveAccountService::OnAccountStateChanged() {
  const auto account_state = account_state_prefs_.GetAccountState();
  EnsureState(account_state->which());

  for (auto& observer : observers_) {
    observer->OnAccountStateChanged(account_state.Clone());
  }
}

void BraveAccountService::EnsureState(mojom::AccountState::Tag which) {
  // NOLINTBEGIN(readability/braces) - false positive on templated lambda.
  const auto ensure_state_fn = [&]<typename State>() {
    if (std::holds_alternative<State>(state_)) {
      return;
    }

    // The receivers to bind to the new state come from either
    // `pending_receivers_` (initial transition from `std::monostate`, where
    // `BindInterface()` queued them while waiting for
    // `FinishInitialization()`), or the outgoing state (state-to-state
    // transition). See class comment in the header for the full migration
    // story.
    auto receivers = std::visit(
        absl::Overload{[](StateBase& state) { return state.TakeReceivers(); },
                       [&](std::monostate) {
                         return std::exchange(pending_receivers_, {});
                       }},
        state_);
    state_.emplace<State>(account_state_prefs_, url_loader_factory_,
                          *encryptor_,
                          base::BindRepeating(&BraveAccountService::AddObserver,
                                              base::Unretained(this)));
    for (auto& receiver : receivers) {
      ActiveState().AddReceiver(std::move(receiver));
    }
  };
  // NOLINTEND(readability/braces)

  switch (which) {
    case mojom::AccountState::Tag::kLoggedOut:
      return ensure_state_fn.operator()<LoggedOutState>();
    case mojom::AccountState::Tag::kLoggedIn:
      return ensure_state_fn.operator()<LoggedInState>();
  }
}

StateBase& BraveAccountService::ActiveState() {
  return std::visit(
      absl::Overload{[](StateBase& state) -> StateBase& { return state; },
                     [](std::monostate) -> StateBase& {
                       // `BindInterface()` queues receivers and
                       // `FinishInitialization()` drains them
                       // only after emplacing the initial state,
                       // so the variant cannot hold
                       // `std::monostate` at this point.
                       NOTREACHED();
                     }},
      state_);
}

}  // namespace brave_account
