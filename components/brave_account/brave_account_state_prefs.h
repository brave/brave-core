/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_STATE_PREFS_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_STATE_PREFS_H_

#include <concepts>
#include <string>
#include <utility>

#include "base/check_deref.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/pref_names.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"

namespace brave_account {

// Typed view onto the kBraveAccountState pref dict.
// Encapsulates the storage schema so callers (the service, tests)
// don't have to know how the dict is laid out.
class AccountStatePrefs {
 public:
  explicit AccountStatePrefs(PrefService& pref_service)
      : pref_service_(pref_service) {}

  AccountStatePrefs(const AccountStatePrefs&) = delete;
  AccountStatePrefs& operator=(const AccountStatePrefs&) = delete;

  void StartObserving(base::RepeatingClosure on_change);

  void SetLoggedOut();

  void SetLoggedOutWithVerification(
      const std::string& encrypted_verification_token,
      mojom::LoggedOutVerificationIntent intent);

  void SetLoggedIn(const std::string& email,
                   const std::string& encrypted_authentication_token);

  mojom::AccountStatePtr GetAccountState() const;

  std::string GetAuthenticationToken() const;

  template <typename Intent>
    requires std::same_as<Intent, mojom::LoggedOutVerificationIntent> ||
             std::same_as<Intent, mojom::LoggedInVerificationIntent>
  std::string GetVerificationToken(Intent intent) const {
    const auto verification = [&] {
      const auto account_state = GetAccountState();
      if constexpr (std::same_as<Intent, mojom::LoggedOutVerificationIntent>) {
        return account_state->is_logged_out()
                   ? std::move(account_state->get_logged_out()->verification)
                   : nullptr;
      } else {
        return account_state->is_logged_in()
                   ? std::move(account_state->get_logged_in()->verification)
                   : nullptr;
      }
    }();

    if (!verification || verification->intent != intent) {
      return "";
    }

    const auto* token =
        CHECK_DEREF(pref_service_->GetDict(prefs::kBraveAccountState)
                        .FindDict(prefs::keys::kVerification))
            .FindString(prefs::keys::kVerificationToken);
    return token ? *token : "";
  }

  std::string GetCachedServiceToken(const std::string& service_name) const;

  void UpdateEmail(const std::string& email);

  void CacheServiceToken(const std::string& service_name,
                         std::string encrypted_service_token);

 private:
  const raw_ref<PrefService> pref_service_;
  PrefChangeRegistrar pref_change_registrar_;
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_STATE_PREFS_H_
