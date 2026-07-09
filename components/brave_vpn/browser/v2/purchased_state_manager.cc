/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/purchased_state_manager.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/notimplemented.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_vpn/browser/v2/credential_store.h"
#include "brave/components/brave_vpn/browser/v2/skus_service_client.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_vpn::v2 {

PurchasedStateManager::PurchasedStateManager(
    PrefService* local_prefs,
    SkusServiceClient* skus_client,
    PurchasedStateChangedCallback callback)
    : local_prefs_(CHECK_DEREF(local_prefs)),
      skus_client_(CHECK_DEREF(skus_client)),
      purchased_state_changed_callback_(std::move(callback)),
      credential_store_(std::make_unique<CredentialStore>(local_prefs)) {
  CHECK(purchased_state_changed_callback_);
  CheckInitialState();
}

PurchasedStateManager::~PurchasedStateManager() = default;

void PurchasedStateManager::Reload() {
  Load(skus::GetDomain(skus::GetVpnProductPrefix(), GetCurrentEnvironment()));
}

void PurchasedStateManager::Load(const std::string& domain) {
  if (!skus::DomainIsForProduct(domain, skus::GetVpnProductPrefix())) {
    VLOG(2) << __func__ << ": Called for non-vpn product";
    return;
  }

  // A load for this environment is already in flight: duplicate, ignore.
  // Loads for any other environment are deliberately not deduped, and cancelled
  // instead: last call wins.
  const std::string request_environment = skus::GetEnvironmentForDomain(domain);
  if (loading_environment_ == request_environment) {
    VLOG(2) << __func__ << ": Already loading the same environment";
    return;
  }

  // Cache fast paths apply only to the current environment; any other
  // environment must authorize with SKUS from scratch.
  const std::string current_environment = GetCurrentEnvironment();
  if (current_environment == request_environment) {
    if (credential_store_->HasValidSubscriberCredential()) {
      // Already purchased. Serving from cache settles the visible state
      // immediately, so any in-flight load for another environment is cancelled
      // rather than left to finish against a state that just changed under it.
      VLOG(2) << "Already have valid subscriber credential, scheduling refresh";
      CancelPendingLoad();
      ScheduleSubscriberCredentialRefresh();
      SetPurchasedState(request_environment, mojom::PurchasedState::PURCHASED);
      return;
    }

    if (credential_store_->HasValidSkusCredential()) {
      // Previous attempt to exchange the skus credential for a subscriber
      // credential failed. Try again with the cached skus credential.
      VLOG(2) << "Trying to exchange cached skus credential for subscriber "
                 "credential";
      BeginLoad(request_environment);

      // TODO(https://github.com/brave/brave-browser/issues/54600)
      // The API request class is intentionally not wired up yet. Once it is,
      // this will call GetSubscriberCredentialV12, binding the current loading
      // sequence into the callback.
      NOTIMPLEMENTED();
      return;
    }
  }

  // No valid credentials, or a different environment: start a fresh resolution.
  // This cancels any in-flight load.
  BeginLoad(request_environment);

  // The response arrives at OnCredentialSummary; its handling is not
  // implemented yet, so until then the load settles via the timeout started in
  // BeginLoad().
  VLOG(2) << "Requesting credential summary from SKUS";
  RequestCredentialSummary(domain);
}

mojom::PurchasedInfo PurchasedStateManager::GetInfo() const {
  return purchased_state_.value_or(
      mojom::PurchasedInfo(mojom::PurchasedState::NOT_PURCHASED, std::nullopt));
}

bool PurchasedStateManager::IsPurchased() const {
  return GetInfo().state == mojom::PurchasedState::PURCHASED;
}

std::string PurchasedStateManager::GetCurrentEnvironment() const {
  return local_prefs_->GetString(prefs::kBraveVPNEnvironment);
}

void PurchasedStateManager::SetPurchasedState(
    const std::string& env,
    mojom::PurchasedState state,
    std::optional<std::string> description) {
  if (purchased_state_.has_value() && purchased_state_->state == state &&
      purchased_state_->description == description) {
    return;
  }
  if (env != GetCurrentEnvironment()) {
    VLOG(2) << "Ignoring purchased state change to " << state
            << " for non-current environment " << env;
    return;
  }
  VLOG(2) << "Changing purchased state to " << state;
  purchased_state_ = mojom::PurchasedInfo(state, description);

  // While the state change itself is synchronous, the notification is posted,
  // so observers always run on a clean stack: SetPurchasedState is reachable
  // from the constructor via CheckInitialState(), and mid-load, and observers
  // may call back into the service or the manager, creating a re-entrancy
  // hazard.
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&PurchasedStateManager::RunPurchasedStateCallback,
                     weak_factory_.GetWeakPtr(), state,
                     std::move(description)));
}

void PurchasedStateManager::CheckInitialState() {
  if (credential_store_->HasValidSubscriberCredential()) {
    // Have a valid subscriber credential, so we are purchased. Schedule a
    // refresh of the credential before it expires.
    VLOG(2) << "Have valid subscriber credential, scheduling refresh";
    ScheduleSubscriberCredentialRefresh();
    SetPurchasedState(GetCurrentEnvironment(),
                      mojom::PurchasedState::PURCHASED);
  } else if (credential_store_->HasValidSkusCredential()) {
    // There is a cached SKUS credential - exchange it for a subscriber
    // credential upfront.
    VLOG(2) << "Reloading purchased state due to cached SKUS credential";
    Reload();
  } else {
    // A stored subscriber credential may have been invalidated while we were
    // not running. Always clear whatever is cached; if something stale was
    // present, reload the state.
    const bool has_stale_credential = credential_store_->HasAnyCredential();
    credential_store_->Clear();
    if (has_stale_credential) {
      VLOG(2) << "Reloading purchased state due to stale credential";
      Reload();
    }
  }
}

void PurchasedStateManager::BeginLoad(std::string env) {
  // Cancels any in-flight load: bumping the loading sequence orphans its
  // response callbacks.
  ++loading_sequence_;
  loading_environment_ = std::move(env);
  // New resolution cycle: reset the exchange retry flag.
  credential_store_->SetExchangeRetried(false);

  // Start a backstop timer to prevent a load from hanging indefinitely.
  load_timeout_timer_.Start(
      FROM_HERE, kLoadTimeout,
      base::BindOnce(&PurchasedStateManager::OnLoadTimeout,
                     base::Unretained(this)));

  // Only a load for the current environment is visible; a load for a different
  // environment runs silently and commits nothing until it obtains a valid SKUS
  // credential for that environment.
  if (loading_environment_ == GetCurrentEnvironment()) {
    SetPurchasedState(loading_environment_, mojom::PurchasedState::LOADING);
  }
}

void PurchasedStateManager::FinishLoad(std::string env,
                                       mojom::PurchasedState state,
                                       std::optional<std::string> description) {
  // A terminal outcome invalidates any response still in flight for this load.
  ++loading_sequence_;
  loading_environment_.clear();
  load_timeout_timer_.Stop();
  SetPurchasedState(env, state, std::move(description));

  // A silent (non-current-environment) load ended without committing. If it
  // previously cancelled a visible load, the visible state may be stranded at
  // LOADING with nothing left in flight to settle it. Reload the current
  // environment.
  if (env != GetCurrentEnvironment() &&
      GetInfo().state == mojom::PurchasedState::LOADING) {
    VLOG(2) << "Reloading current environment after silent load ended";
    Reload();
  }
}

void PurchasedStateManager::CancelPendingLoad() {
  if (!loading_environment_.empty()) {
    ++loading_sequence_;
    loading_environment_.clear();
    load_timeout_timer_.Stop();
  }
}

void PurchasedStateManager::OnLoadTimeout() {
  DCHECK(!loading_environment_.empty());
  VLOG(1) << "Purchased state load timed out for environment "
          << loading_environment_;
  FinishLoad(loading_environment_, mojom::PurchasedState::FAILED,
             l10n_util::GetStringUTF8(
                 IDS_BRAVE_VPN_PURCHASE_CREDENTIALS_FETCH_FAILED));
}

void PurchasedStateManager::RequestCredentialSummary(
    const std::string& domain) {
  // Call SKUS to get a credential summary for the domain.
  skus_client_->GetCredentialSummary(
      domain,
      base::BindOnce(&PurchasedStateManager::OnCredentialSummary,
                     weak_factory_.GetWeakPtr(), loading_sequence_, domain));
}

void PurchasedStateManager::OnCredentialSummary(
    uint64_t sequence,
    const std::string& domain,
    skus::mojom::SkusResultPtr summary) {
  if (sequence != loading_sequence_) {
    VLOG(2) << __func__ << ": Ignoring response of a stale load";
    return;
  }
  if (!skus::DomainIsForProduct(domain, skus::GetVpnProductPrefix())) {
    VLOG(2) << __func__ << ": Called for non-vpn product";
    return;
  }

  // NOTE: when implemented, (1) every terminal outcome must go through
  // FinishLoad(); (2) non-purchased outcomes must Clear() the credential
  // store, or stale credentials resurrect via CheckInitialState() on next
  // launch; (3) transport errors (the synthesized disconnect result) settle
  // as FAILED with no immediate Reload() - that would retry-storm a dead
  // service.
  NOTIMPLEMENTED();
}

void PurchasedStateManager::RunPurchasedStateCallback(
    mojom::PurchasedState state,
    std::optional<std::string> description) {
  purchased_state_changed_callback_.Run(state, std::move(description));
}

void PurchasedStateManager::ScheduleSubscriberCredentialRefresh() {
  NOTIMPLEMENTED();
}

}  // namespace brave_vpn::v2
