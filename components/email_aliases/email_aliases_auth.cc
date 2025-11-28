/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/email_aliases/email_aliases_auth.h"

#include "base/auto_reset.h"
#include "base/base64.h"
#include "components/os_crypt/sync/os_crypt.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace {

bool Encrypt(const std::string& plain_text, std::string& out) {
  if (plain_text.empty()) {
    return false;
  }

  if (!OSCrypt::EncryptString(plain_text, &out)) {
    return false;
  }

  out = base::Base64Encode(out);
  return true;
}

bool Decrypt(const std::string& base64, std::string& out) {
  if (base64.empty()) {
    return false;
  }

  std::string encrypted;
  if (!base::Base64Decode(base64, &encrypted)) {
    return false;
  }

  if (!OSCrypt::DecryptString(encrypted, &out)) {
    return false;
  }

  return true;
}

}  // namespace

namespace email_aliases {

EmailAliasesAuth::EmailAliasesAuth(PrefService* prefs_service,
                                   OnChangedCallback on_changed)
    : on_changed_(std::move(on_changed)) {
  CHECK(prefs_service);
  CHECK(on_changed_);

  pref_auth_email_.Init(prefs::kBaseEmail, prefs_service,
                        base::BindRepeating(&EmailAliasesAuth::OnPrefChanged,
                                            base::Unretained(this)));
  pref_auth_token_.Init(prefs::kAuthToken, prefs_service,
                        base::BindRepeating(&EmailAliasesAuth::OnPrefChanged,
                                            base::Unretained(this)));
}

EmailAliasesAuth::~EmailAliasesAuth() = default;

// static
void EmailAliasesAuth::RegisterProfilePref(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(prefs::kBaseEmail, {});
  registry->RegisterStringPref(prefs::kAuthToken, {});
}

bool EmailAliasesAuth::IsAuthenticated() const {
  return !pref_auth_email_.GetValue().empty() &&
         !pref_auth_token_.GetValue().empty();
}

void EmailAliasesAuth::SetAuthEmail(const std::string& base_email) {
  if (pref_auth_email_.GetValue() != base_email) {
    base::AutoReset<bool> notifyonce(&notify_, false);  // notify once
    pref_auth_token_.SetValue({});
    pref_auth_email_.SetValue(base_email);
    on_changed_.Run();
  }
}

void EmailAliasesAuth::SetAuthToken(const std::string& auth_token) {
  std::string encrypted;
  if (auth_token.empty() || !Encrypt(auth_token, encrypted)) {
    pref_auth_token_.SetValue({});
  } else {
    pref_auth_token_.SetValue(encrypted);
  }
  base::AutoReset<bool> reenter(
      &notify_, false);  // Do not renotificate if callback changes prefs.
  on_changed_.Run();
}

std::string EmailAliasesAuth::GetAuthEmail() const {
  return pref_auth_email_.GetValue();
}

std::string EmailAliasesAuth::CheckAndGetAuthToken() {
  const std::string encrypted = pref_auth_token_.GetValue();
  std::string token;

  if (encrypted.empty()) {
    return {};
  }

  if (!Decrypt(encrypted, token)) {
    // Failed to decrypt token -> reset.
    SetAuthToken({});
    return {};
  }
  return token;
}

void EmailAliasesAuth::OnPrefChanged(const std::string& pref_name) {
  if (notify_) {
    base::AutoReset<bool> reenter(
        &notify_, false);  // Do not renotificate if callback changes prefs.
    CheckAndGetAuthToken();
    on_changed_.Run();
  }
}

}  // namespace email_aliases
