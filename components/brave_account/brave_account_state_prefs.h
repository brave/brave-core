/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_STATE_PREFS_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_STATE_PREFS_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
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

  void SetLoggedIn(const std::string& email,
                   const std::string& encrypted_authentication_token);

  // Attaches a verification slot to the current state without changing whether
  // the user is logged in or out. `intent`'s tag (logged-out/logged-in)
  // selects the state the slot belongs to and must match the current state -
  // CHECK()s otherwise.
  void AddVerification(const std::string& encrypted_verification_token,
                       mojom::VerificationIntentPtr intent);

  // Records the verified email on the current verification slot.
  // Called once the email is verified (OTP step completed).
  // Requires a verification slot to be present.
  void SetVerificationVerifiedEmail(
      const std::string& verification_verified_email);

  // Removes the verification slot from the current state without changing
  // whether the user is logged in or out. No-op if no slot is present.
  void ClearVerification();

  mojom::AccountStatePtr GetAccountState() const;

  std::string GetAuthenticationToken() const;

  // Returns the encrypted verification token if a verification matching
  // `intent` is currently pending, or "" otherwise.
  std::string GetVerificationToken(mojom::VerificationIntentPtr intent) const;

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
