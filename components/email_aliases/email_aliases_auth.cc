/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/email_aliases/email_aliases_auth.h"

#include "base/check_deref.h"
#include "base/check_is_test.h"
#include "brave/components/brave_account/pref_names.h"
#include "components/prefs/pref_service.h"

namespace email_aliases {

EmailAliasesAuth::EmailAliasesAuth(
    PrefService* prefs_service,
    brave_account::mojom::Authentication* brave_account_auth,
    OnChangedCallback on_changed)
    : prefs_service_(prefs_service),
      brave_account_auth_(brave_account_auth),
      on_changed_(std::move(on_changed)) {
  CHECK(prefs_service_);
  CHECK(brave_account_auth_);
  CHECK(on_changed_);

  pref_change_registrar_.Init(prefs_service_);
  pref_change_registrar_.Add(
      brave_account::prefs::kBraveAccountServiceTokens,
      base::BindRepeating(&EmailAliasesAuth::OnPrefChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      brave_account::prefs::kBraveAccountEmailAddress,
      base::BindRepeating(&EmailAliasesAuth::OnPrefChanged,
                          base::Unretained(this)));
}

EmailAliasesAuth::~EmailAliasesAuth() = default;

bool EmailAliasesAuth::IsAuthenticated() const {
  return !GetAuthEmail().empty();
}

std::string EmailAliasesAuth::GetAuthEmail() const {
  if (auth_email_for_testing_) {
    CHECK_IS_TEST();
    return auth_email_for_testing_.value();
  }
  return prefs_service_->GetString(
      brave_account::prefs::kBraveAccountEmailAddress);
}

void EmailAliasesAuth::GetServiceToken(
    brave_account::mojom::Authentication::GetServiceTokenCallback callback) {
  CHECK_DEREF(brave_account_auth_)
      .GetServiceToken(brave_account::mojom::Service::kEmailAliases,
                       std::move(callback));
}

void EmailAliasesAuth::SetAuthEmailForTesting(const std::string& email) {
  auth_email_for_testing_ = email;
  on_changed_.Run();
}

void EmailAliasesAuth::OnPrefChanged(const std::string& pref_name) {
  on_changed_.Run();
}

}  // namespace email_aliases
