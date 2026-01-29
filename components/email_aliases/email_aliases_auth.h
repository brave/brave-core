/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_AUTH_H_
#define BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_AUTH_H_

#include <optional>

#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_member.h"

class PrefRegistrySimple;
class PrefService;

namespace email_aliases {

class EmailAliasesAuth {
 public:
  using OnChangedCallback = base::RepeatingClosure;

  explicit EmailAliasesAuth(
      PrefService* prefs_service,
      brave_account::mojom::Authentication* brave_account_auth,
      OnChangedCallback on_changed = base::DoNothing());
  ~EmailAliasesAuth();

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  bool IsAuthenticated() const;
  std::string GetAuthEmail() const;
  void GetServiceToken(
      brave_account::mojom::Authentication::GetServiceTokenCallback callback);

  void SetAuthEmailForTesting(const std::string& email);

 private:
  void OnPrefChanged(const std::string& pref_name);

  std::optional<std::string> auth_email_for_testing_;

  const raw_ptr<PrefService> prefs_service_ = nullptr;
  const raw_ptr<brave_account::mojom::Authentication> brave_account_auth_ =
      nullptr;

  PrefChangeRegistrar pref_change_registrar_;
  OnChangedCallback on_changed_;
};

}  // namespace email_aliases

#endif  // BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_AUTH_H_
