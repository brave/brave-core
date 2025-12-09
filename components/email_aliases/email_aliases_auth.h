/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_AUTH_H_
#define BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_AUTH_H_

#include "components/os_crypt/async/common/encryptor.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_member.h"

class PrefRegistrySimple;
class PrefService;

namespace email_aliases {

namespace prefs {

inline constexpr char kAuth[] = "brave.email_aliases.auth";

}  // namespace prefs

class EmailAliasesAuth {
 public:
  using OnChangedCallback = base::RepeatingClosure;

  explicit EmailAliasesAuth(PrefService* prefs_service,
                            os_crypt_async::Encryptor encryptor,
                            OnChangedCallback on_changed = base::DoNothing());
  ~EmailAliasesAuth();

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  bool IsAuthenticated() const;

  void SetAuthEmail(const std::string& email);
  void SetAuthToken(const std::string& auth_token);

  std::string GetAuthEmail() const;
  std::string CheckAndGetAuthToken();

 private:
  void OnPrefChanged(const std::string& pref_name);

  const raw_ptr<PrefService> prefs_service_ = nullptr;

  os_crypt_async::Encryptor encryptor_;

  PrefChangeRegistrar pref_change_registrar_;

  OnChangedCallback on_changed_;

  bool notify_ = true;

  std::string auth_email_;
  bool is_authenticated_ = false;
};

}  // namespace email_aliases

#endif  // BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_AUTH_H_
