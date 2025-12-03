/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/email_aliases/email_aliases_auth.h"

#include "base/auto_reset.h"
#include "base/base64.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace {

bool Encrypt(const os_crypt_async::Encryptor& encryptor,
             const std::string& plain_text,
             std::string& out) {
  if (plain_text.empty()) {
    return false;
  }

  auto encrypted = encryptor.EncryptString(plain_text);
  if (!encrypted) {
    return false;
  }

  out = base::Base64Encode(encrypted.value());
  return true;
}

bool Decrypt(const os_crypt_async::Encryptor& encryptor,
             const std::string& base64,
             std::string& out) {
  if (base64.empty()) {
    return false;
  }

  auto encrypted = base::Base64Decode(base64);
  if (!encrypted) {
    return false;
  }

  auto decrypted = encryptor.DecryptData(encrypted.value());
  if (!decrypted) {
    return false;
  }
  out = std::string(decrypted->begin(), decrypted->end());
  return true;
}

}  // namespace

namespace email_aliases {

EmailAliasesAuth::EmailAliasesAuth(PrefService* prefs_service,
                                   os_crypt_async::Encryptor encryptor,
                                   OnChangedCallback on_changed)
    : encryptor_(std::move(encryptor)), on_changed_(std::move(on_changed)) {
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
void EmailAliasesAuth::RegisterProfilePrefs(PrefRegistrySimple* registry) {
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
  if (auth_token.empty() || !Encrypt(encryptor_, auth_token, encrypted)) {
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

  if (!Decrypt(encryptor_, encrypted, token)) {
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
