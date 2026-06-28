/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/purchased_state_manager.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/notimplemented.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "brave/components/brave_vpn/browser/v2/credential_store.h"
#include "brave/components/brave_vpn/browser/v2/credential_summary.h"
#include "brave/components/brave_vpn/browser/v2/skus_service_client.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "net/cookies/cookie_inclusion_status.h"
#include "net/cookies/cookie_util.h"
#include "net/cookies/parsed_cookie.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/url_util.h"

namespace brave_vpn::v2 {

PurchasedStateManager::PurchasedStateManager(
    PrefService* local_prefs,
    SkusServiceClient* skus_client,
    PurchasedStateChangedCallback callback)
    : local_prefs_(CHECK_DEREF(local_prefs)),
      skus_client_(CHECK_DEREF(skus_client)),
      purchased_state_changed_callback_(std::move(callback)),
      credential_store_(std::make_unique<CredentialStore>(local_prefs)),
      current_environment_(
          local_prefs_->GetString(prefs::kBraveVPNEnvironment)) {
  // Defer initial state resolution off the constructor, since it can
  // synchronously run a callback which would reenter the still-constructing
  // BraveVpnServiceImpl, and push a change before any client can observe it.
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&PurchasedStateManager::CheckInitialState,
                                weak_factory_.GetWeakPtr()));
}

PurchasedStateManager::~PurchasedStateManager() = default;

void PurchasedStateManager::Reload() {
  Load(skus::GetDomain(skus::GetVpnProductPrefix(), current_environment_));
}

void PurchasedStateManager::Load(const std::string& domain) {
  if (!skus::DomainIsForProduct(domain, skus::GetVpnProductPrefix())) {
    VLOG(2) << __func__ << ": Called for non-vpn product";
    return;
  }

  // Set current environment if not set yet. This can happen if the user has
  // never used VPN before, and the environment is not set in local state.
  const std::string request_environment = skus::GetEnvironmentForDomain(domain);
  if (current_environment_.empty()) {
    current_environment_ = request_environment;
    UpdateCurrentEnvironment();
  }

  // Check if already loading the same environment.
  if (current_environment_ == request_environment &&
      GetInfo().state == mojom::PurchasedState::LOADING) {
    VLOG(2) << __func__ << ": Already loading the same environment";
    return;
  }

  // Clear the retry flag since this is a new resolution cycle.
  credential_store_->SetExchangeRetried(false);

  SetPurchasedState(request_environment, mojom::PurchasedState::LOADING);

  if (credential_store_->HasValidSubscriberCredential()) {
    // Already have a valid subscriber credential, so we are purchased. Schedule
    // a refresh of the credential before it expires.
    VLOG(2) << "Already have a valid subscriber credential, scheduling refresh";
    ScheduleSubscriberCredentialRefresh();
    SetPurchasedState(request_environment, mojom::PurchasedState::PURCHASED);
    return;
  }

  if (credential_store_->HasValidSkusCredential()) {
    // Previous attempt to exchange the skus credential for a subscriber
    // credential failed. Try again with the cached skus credential.
    VLOG(2) << "Trying to exchange cached skus credential for subscriber "
               "credential";

    // TODO(https://github.com/brave/brave-browser/issues/54600)
    // The API request class is intentionally not wired up yet.
    // Once it is, this will call GetSubscriberCredentialV12.
    NOTIMPLEMENTED();
    return;
  }

  // No valid credentials, so request a new credential summary from SKUS.
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
  return current_environment_;
}

void PurchasedStateManager::SetPurchasedState(
    const std::string& env,
    mojom::PurchasedState state,
    const std::optional<std::string>& description) {
  if (GetInfo().state != state && env == current_environment_) {
    VLOG(2) << "Changing purchased state to " << state;
    purchased_state_ = mojom::PurchasedInfo(state, description);
    purchased_state_changed_callback_.Run(state, description);
  }
}

void PurchasedStateManager::CheckInitialState() {
  if (credential_store_->HasValidSubscriberCredential()) {
    // Have a valid subscriber credential, so we are purchased. Schedule a
    // refresh of the credential before it expires.
    VLOG(2) << "Have valid subscriber credential, scheduling refresh";
    ScheduleSubscriberCredentialRefresh();
    SetPurchasedState(current_environment_, mojom::PurchasedState::PURCHASED);
  } else if (credential_store_->HasValidSkusCredential()) {
    // There is a cached SKUS credential - exchange it for a subscriber
    // credential up front.
    VLOG(2) << "Reloading purchased state due to cached SKUS credential";
    Reload();
  } else {
    // A stored subscriber credential may have been invalidated while we were
    // not running. Always clear whatever is cached; if something stale was
    // present, reload to re-derive state.
    const bool has_stale_credential = credential_store_->HasAnyCredential();
    credential_store_->Clear();
    if (has_stale_credential) {
      VLOG(2) << "Reloading purchased state due to stale credential";
      Reload();
    }
  }
}

void PurchasedStateManager::UpdateCurrentEnvironment() {
  local_prefs_->SetString(prefs::kBraveVPNEnvironment, current_environment_);
  purchased_state_.reset();
}

void PurchasedStateManager::RequestCredentialSummary(
    const std::string& domain) {
  // As we request a new credential, clear the cached value.
  credential_store_->Clear();

  skus_client_->GetCredentialSummary(
      domain, base::BindOnce(&PurchasedStateManager::OnCredentialSummary,
                             weak_factory_.GetWeakPtr(), domain));
}

void PurchasedStateManager::OnCredentialSummary(
    const std::string& domain,
    skus::mojom::SkusResultPtr summary) {
  if (!skus::DomainIsForProduct(domain, skus::GetVpnProductPrefix())) {
    VLOG(2) << __func__ << ": Called for non-vpn product";
    return;
  }

  const std::string request_environment = skus::GetEnvironmentForDomain(domain);

  const std::optional<CredentialSummary> parsed =
      CredentialSummary::FromMessage(summary->message);
  if (!parsed) {
    // Non-empty but unparseable summary.
    VLOG(2) << "Got invalid credential summary";
    SetPurchasedState(request_environment, mojom::PurchasedState::FAILED);
    return;
  }

  if (parsed->IsEmpty()) {
    // No subscription on record: the user needs to log in (or never had one).
    VLOG(2) << "Got empty credential summary";
    SetPurchasedState(request_environment,
                      mojom::PurchasedState::NOT_PURCHASED);
    return;
  }

  if (parsed->IsValid()) {
    // Active credential: present it so we can read the credential cookie.
    VLOG(2) << "Got valid credential summary";
    skus_client_->PrepareCredentialsPresentation(
        domain, "*",
        base::BindOnce(&PurchasedStateManager::OnPrepareCredentialsPresentation,
                       weak_factory_.GetWeakPtr(), domain));
#if !BUILDFLAG(IS_ANDROID)
    // Clear expired state data as we have active credentials.
    local_prefs_->SetTime(prefs::kBraveVPNSessionExpiredDate, {});
#endif
    return;
  }

  if (parsed->NeedsActivation()) {
    // Needs activation from account; treat as not purchased until activated.
    VLOG(2) << "Got credential summary but needs activation";
    SetPurchasedState(request_environment,
                      mojom::PurchasedState::NOT_PURCHASED);
    return;
  }

  // Subscription on record but no remaining credential: the purchase is
  // expired.
#if BUILDFLAG(IS_ANDROID)
  SetPurchasedState(request_environment, mojom::PurchasedState::NOT_PURCHASED);
#else
  UpdatePurchasedStateForSessionExpired(request_environment);
#endif
}

void PurchasedStateManager::OnPrepareCredentialsPresentation(
    const std::string& domain,
    skus::mojom::SkusResultPtr credential_as_cookie) {
  if (!skus::DomainIsForProduct(domain, skus::GetVpnProductPrefix())) {
    VLOG(2) << __func__ << ": Called for non-vpn product";
    return;
  }

  const std::string request_environment = skus::GetEnvironmentForDomain(domain);

  // The credential is returned in cookie format.
  net::CookieInclusionStatus status;
  net::ParsedCookie credential_cookie(credential_as_cookie->message, &status);
  if (!credential_cookie.IsValid() || !status.IsInclude() ||
      !credential_cookie.Expires()) {
    VLOG(2) << "Got invalid credential cookie";
    SetPurchasedState(request_environment, mojom::PurchasedState::FAILED);
    return;
  }

  // The value is URL-encoded; decode it to recover the Base64 credential blob.
  const base::Time time =
      net::cookie_util::ParseCookieExpirationTime(*credential_cookie.Expires());
  const std::string credential = url::DecodeUrlEscapeSequences(
      credential_cookie.Value(), url::DecodeUrlMode::kUtf8OrIsomorphic);
  if (credential.empty()) {
    VLOG(2) << "Got empty credential cookie";
    SetPurchasedState(request_environment,
                      mojom::PurchasedState::NOT_PURCHASED);
    return;
  }

  // Already expired.
  if (time < base::Time::Now()) {
    VLOG(2) << "Got expired credential cookie";
    SetPurchasedState(
        request_environment, mojom::PurchasedState::FAILED,
        l10n_util::GetStringUTF8(IDS_BRAVE_VPN_PURCHASE_CREDENTIALS_EXPIRED));
    return;
  }

  // Update the cached skus credential and its expiration time.
  credential_store_->SetSkusCredential(credential, time);

  // We have successfully authorized with a new environment.
  if (current_environment_ != request_environment) {
    current_environment_ = request_environment;
    UpdateCurrentEnvironment();
  }

  // TODO(https://github.com/brave/brave-browser/issues/54600)
  // The API request class is intentionally not wired up yet.
  // Once it is, this will call GetSubscriberCredentialV12.
  NOTIMPLEMENTED();
}

void PurchasedStateManager::OnGetSubscriberCredentialV12(
    base::Time expiration_time,
    const std::string& subscriber_credential,
    bool success) {
  if (!success) {
    VLOG(1) << "Failed to get subscriber credential";
#if BUILDFLAG(IS_ANDROID)
    SetPurchasedState(current_environment_,
                      mojom::PurchasedState::NOT_PURCHASED);
#else   // BUILDFLAG(IS_ANDROID)
    const bool token_no_longer_valid =
        subscriber_credential == ::brave_vpn::kTokenNoLongerValid;

    // "Token no longer valid" means the credential was already consumed. Make
    // one more attempt with a fresh SKUS credential (two attempts total).
    // Set the retried flag before re-requesting so a synchronous callback
    // won't loop forever.
    if (token_no_longer_valid && !credential_store_->IsExchangeRetried()) {
      VLOG(2) << "Token no longer valid, retrying with fresh SKUS credential";
      credential_store_->SetExchangeRetried(true);
      RequestCredentialSummary(
          skus::GetDomain(skus::GetVpnProductPrefix(), current_environment_));
      return;
    }

    // The retry also came back invalid: the token is definitively no good.
    if (token_no_longer_valid) {
      VLOG(2) << "Token no longer valid after retry";
      SetPurchasedState(
          current_environment_, mojom::PurchasedState::FAILED,
          l10n_util::GetStringUTF8(IDS_BRAVE_VPN_PURCHASE_TOKEN_NOT_VALID));
      return;
    }

    // Otherwise it's a transient vendor/network error. The cached credential
    // will eventually expire and a new one will be fetched.
    VLOG(2) << "Transient error, will retry later";
    SetPurchasedState(current_environment_, mojom::PurchasedState::FAILED,
                      l10n_util::GetStringUTF8(
                          IDS_BRAVE_VPN_PURCHASE_CREDENTIALS_FETCH_FAILED));
#endif  // BUILDFLAG(IS_ANDROID)
    return;
  }

  // Got a valid subscriber credential. Clear the retry flag, cache it, and
  // schedule a refresh before it expires.
  credential_store_->SetExchangeRetried(false);
  credential_store_->SetSubscriberCredential(subscriber_credential,
                                             expiration_time);
  ScheduleSubscriberCredentialRefresh();
  SetPurchasedState(current_environment_, mojom::PurchasedState::PURCHASED);
}

void PurchasedStateManager::ScheduleSubscriberCredentialRefresh() {
  if (subs_cred_refresh_timer_.IsRunning()) {
    subs_cred_refresh_timer_.Stop();
  }

  const std::optional<base::Time> expiration_time =
      credential_store_->GetExpirationTime();
  if (!expiration_time) {
    VLOG(2) << "No expiration time available for subscriber credential";
    return;
  }

  // Safe to pass unretained here, because the timer is a member and cannot
  // outlive the manager.
  subs_cred_refresh_timer_.Start(
      FROM_HERE, *expiration_time - base::Time::Now(),
      base::BindOnce(&PurchasedStateManager::RefreshSubscriberCredential,
                     base::Unretained(this)));
}

void PurchasedStateManager::RefreshSubscriberCredential() {
  VLOG(2) << "Refresh subscriber credential";
  // Clear current credentials to get newer one.
  credential_store_->Clear();
  Reload();
}

#if !BUILDFLAG(IS_ANDROID)

void PurchasedStateManager::UpdatePurchasedStateForSessionExpired(
    const std::string& env) {
  // TODO(https://github.com/brave/brave-browser/issues/54601)
  // BraveVpnService v1 first guarded against marking a *fresh* user as
  // session-expired by checking connection-manager region-data readiness
  // (treating "no region data" as a brand-new user -> NOT_PURCHASED). In v2,
  // until the agent is properly plumbed, a brand-new user who reaches this path
  // would be reported SESSION_EXPIRED rather than NOT_PURCHASED.

  // If the last credential expiry is in the future, the user redeemed their
  // credentials but we lost communication with the vendor: out of credentials.
  // kBraveVPNLastCredentialExpiry is only set after obtaining a valid
  // credential, so checking it first is safe.
  const base::Time last_credential_expiry =
      local_prefs_->GetTime(prefs::kBraveVPNLastCredentialExpiry);
  const base::Time current_time = base::Time::Now();
  if (!last_credential_expiry.is_null() &&
      last_credential_expiry > current_time) {
    base::TimeDelta delta = (last_credential_expiry - current_time);
    if (delta.InHours() == 0) {
      VLOG(2) << "Out of credentials; check again in " << delta.InMinutes()
              << " minutes.";
    } else {
      const int delta_hours = delta.InHours();
      base::TimeDelta delta_minutes = (delta - base::Hours(delta_hours));
      VLOG(2) << "Out of credentials; check again in " << delta_hours
              << " hours " << delta_minutes.InMinutes() << " minutes.";
    }
    SetPurchasedState(env, mojom::PurchasedState::OUT_OF_CREDENTIALS,
                      l10n_util::GetStringUTF8(
                          IDS_BRAVE_VPN_MAIN_PANEL_OUT_OF_CREDENTIALS_CONTENT));
    return;
  }

  const base::Time session_expired_time =
      local_prefs_->GetTime(prefs::kBraveVPNSessionExpiredDate);
  // First session expiration since the last purchase (this pref is cleared when
  // we get a valid credential summary): stamp it now and report expired.
  if (session_expired_time.is_null()) {
    VLOG(2) << "First session expiration since last purchase";
    local_prefs_->SetTime(prefs::kBraveVPNSessionExpiredDate,
                          base::Time::Now());
    SetPurchasedState(env, mojom::PurchasedState::SESSION_EXPIRED);
    return;
  }

  // Stamp in the future (clock weirdness): treat as not purchased.
  if (session_expired_time > base::Time::Now()) {
    VLOG(2) << "Session expired time is in the future";
    SetPurchasedState(env, mojom::PurchasedState::NOT_PURCHASED);
    return;
  }

  // If the session-expired state has been held for more than the configured
  // duration, treat as not purchased.
  constexpr int kSessionExpiredCheckingDurationInDays = 30;
  if ((base::Time::Now() - session_expired_time).InDays() >
      kSessionExpiredCheckingDurationInDays) {
    VLOG(2) << "Session expired state held for more than "
            << kSessionExpiredCheckingDurationInDays << " days";
    SetPurchasedState(env, mojom::PurchasedState::NOT_PURCHASED);
    return;
  }

  // Expiry is in the past - the user must log into account.brave.com again.
  VLOG(2) << "Session expired, user must log in again";
  SetPurchasedState(env, mojom::PurchasedState::SESSION_EXPIRED);
}

#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace brave_vpn::v2
