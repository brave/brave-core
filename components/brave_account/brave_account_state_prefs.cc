/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/brave_account_state_prefs.h"

#include <optional>

#include "base/check.h"
#include "base/json/values_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_account/brave_account_service_constants.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_account {

namespace {

template <typename VerificationPtr>
auto MakeVerification(std::optional<int> verification_intent) {
  VerificationPtr verification;

  if (verification_intent) {
    if (const auto intent =
            static_cast<decltype(verification->intent)>(*verification_intent);
        mojom::IsKnownEnumValue(intent)) {
      verification = VerificationPtr::Struct::New(intent);
    }
  }

  return verification;
}

}  // namespace

void AccountStatePrefs::StartObserving(base::RepeatingClosure on_change) {
  pref_change_registrar_.Init(&*pref_service_);
  pref_change_registrar_.Add(prefs::kBraveAccountState, std::move(on_change));
}

void AccountStatePrefs::SetLoggedOut() {
  pref_service_->SetDict(prefs::kBraveAccountState,
                         base::DictValue().Set(prefs::keys::kKind,
                                               prefs::state_kinds::kLoggedOut));
}

void AccountStatePrefs::SetLoggedOutWithVerification(
    const std::string& encrypted_verification_token,
    mojom::LoggedOutVerificationIntent intent) {
  CHECK(!encrypted_verification_token.empty());

  pref_service_->SetDict(
      prefs::kBraveAccountState,
      base::DictValue()
          .Set(prefs::keys::kKind, prefs::state_kinds::kLoggedOut)
          .Set(prefs::keys::kVerification,
               base::DictValue()
                   .Set(prefs::keys::kVerificationToken,
                        encrypted_verification_token)
                   .Set(prefs::keys::kVerificationIntent,
                        static_cast<int>(intent))));
}

void AccountStatePrefs::SetLoggedIn(
    const std::string& email,
    const std::string& encrypted_authentication_token) {
  CHECK(!email.empty());
  CHECK(!encrypted_authentication_token.empty());

  pref_service_->SetDict(
      prefs::kBraveAccountState,
      base::DictValue()
          .Set(prefs::keys::kKind, prefs::state_kinds::kLoggedIn)
          .Set(prefs::keys::kEmail, email)
          .Set(prefs::keys::kAuthenticationToken,
               encrypted_authentication_token));
}

mojom::AccountStatePtr AccountStatePrefs::GetAccountState() const {
  const auto& account_state = pref_service_->GetDict(prefs::kBraveAccountState);
  const auto* kind = account_state.FindString(prefs::keys::kKind);
  const auto* verification = account_state.FindDict(prefs::keys::kVerification);
  const auto intent =
      verification ? verification->FindInt(prefs::keys::kVerificationIntent)
                   : std::nullopt;

  if (kind && *kind == prefs::state_kinds::kLoggedIn) {
    const auto* email = account_state.FindString(prefs::keys::kEmail);
    CHECK(email && !email->empty());
    return mojom::AccountState::NewLoggedIn(mojom::LoggedInState::New(
        *email, MakeVerification<mojom::LoggedInVerificationPtr>(intent)));
  }

  return mojom::AccountState::NewLoggedOut(mojom::LoggedOutState::New(
      MakeVerification<mojom::LoggedOutVerificationPtr>(intent)));
}

std::string AccountStatePrefs::GetAuthenticationToken() const {
  const auto* token = pref_service_->GetDict(prefs::kBraveAccountState)
                          .FindString(prefs::keys::kAuthenticationToken);
  return token ? *token : "";
}

std::string AccountStatePrefs::GetCachedServiceToken(
    const std::string& service_name) const {
  CHECK(!service_name.empty());

  const auto* service_tokens = pref_service_->GetDict(prefs::kBraveAccountState)
                                   .FindDict(prefs::keys::kServiceTokens);
  const auto* service =
      service_tokens ? service_tokens->FindDict(service_name) : nullptr;
  if (!service) {
    return "";
  }

  const auto* encrypted_service_token =
      service->FindString(prefs::keys::kServiceToken);
  const auto* last_fetched_value = service->Find(prefs::keys::kLastFetched);

  if (!encrypted_service_token || !last_fetched_value) {
    return "";
  }

  const auto last_fetched_time = base::ValueToTime(*last_fetched_value);
  if (!last_fetched_time) {
    return "";
  }

  if (base::Time::Now() - *last_fetched_time >= kServiceTokenMaxAge) {
    return "";
  }

  return *encrypted_service_token;
}

void AccountStatePrefs::UpdateEmail(const std::string& email) {
  CHECK(!email.empty());

  ScopedDictPrefUpdate(&*pref_service_, prefs::kBraveAccountState)
      ->Set(prefs::keys::kEmail, email);
}

void AccountStatePrefs::CacheServiceToken(const std::string& service_name,
                                          std::string encrypted_service_token) {
  CHECK(!service_name.empty());
  CHECK(!encrypted_service_token.empty());

  ScopedDictPrefUpdate(&*pref_service_, prefs::kBraveAccountState)
      ->EnsureDict(prefs::keys::kServiceTokens)
      ->Set(service_name, base::DictValue()
                              .Set(prefs::keys::kServiceToken,
                                   std::move(encrypted_service_token))
                              .Set(prefs::keys::kLastFetched,
                                   base::TimeToValue(base::Time::Now())));
}

}  // namespace brave_account
