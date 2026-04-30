/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_STATE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_STATE_H_

#include <concepts>
#include <string>
#include <utility>

#include "base/check_deref.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/pref_names.h"
#include "components/prefs/pref_service.h"

namespace brave_account::internal {

void SetLoggedOut(PrefService& pref_service);

void SetLoggedOutWithVerification(
    PrefService& pref_service,
    const std::string& encrypted_verification_token,
    mojom::LoggedOutVerificationIntent intent);

void SetLoggedIn(PrefService& pref_service,
                 const std::string& email,
                 const std::string& encrypted_authentication_token);

mojom::AccountStatePtr GetAccountState(const PrefService& pref_service);

std::string GetAuthenticationToken(const PrefService& pref_service);

template <typename Intent>
  requires std::same_as<Intent, mojom::LoggedOutVerificationIntent> ||
           std::same_as<Intent, mojom::LoggedInVerificationIntent>
std::string GetVerificationToken(const PrefService& pref_service,
                                 Intent intent) {
  const auto verification = [&] {
    const auto state = GetAccountState(pref_service);
    if constexpr (std::same_as<Intent, mojom::LoggedOutVerificationIntent>) {
      return state->is_logged_out()
                 ? std::move(state->get_logged_out()->verification)
                 : nullptr;
    } else {
      return state->is_logged_in()
                 ? std::move(state->get_logged_in()->verification)
                 : nullptr;
    }
  }();

  if (!verification || verification->intent != intent) {
    return "";
  }

  const auto* token =
      CHECK_DEREF(pref_service.GetDict(prefs::kBraveAccountState)
                      .FindDict(prefs::keys::kVerification))
          .FindString(prefs::keys::kVerificationToken);
  return token ? *token : "";
}

std::string GetCachedServiceToken(const PrefService& pref_service,
                                  const std::string& service_name);

void UpdateEmail(PrefService& pref_service, const std::string& email);

void CacheServiceToken(PrefService& pref_service,
                       const std::string& service_name,
                       std::string encrypted_service_token);

}  // namespace brave_account::internal

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_STATE_H_
