/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/email_aliases/email_aliases_auth.h"

#include "base/auto_reset.h"
#include "base/base64.h"
#include "brave/components/brave_account/pref_names.h"
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
    : prefs_service_(prefs_service),
      encryptor_(std::move(encryptor)),
      on_changed_(std::move(on_changed)) {
  CHECK(prefs_service_);
  CHECK(on_changed_);

  pref_change_registrar_.Init(prefs_service_);
  pref_change_registrar_.Add(
      brave_account::prefs::kBraveAccountAuthenticationToken,
      base::BindRepeating(&EmailAliasesAuth::OnPrefChanged,
                          base::Unretained(this)));

  auth_email_ =
      prefs_service->GetString(brave_account::prefs::kBraveAccountEmailAddress);
  is_authenticated_ = !CheckAndGetAuthToken().empty() && !auth_email_.empty();
}

EmailAliasesAuth::~EmailAliasesAuth() = default;

bool EmailAliasesAuth::IsAuthenticated() const {
  return is_authenticated_;
}

void EmailAliasesAuth::SetAuthEmail(const std::string& email) {
  if (GetAuthEmail() != email) {
    auth_email_ = email;
    SetAuthToken({});
  }
}

void EmailAliasesAuth::SetAuthToken(const std::string& auth_token) {
  std::string encrypted;
  if (auth_token.empty() || !Encrypt(encryptor_, auth_token, encrypted)) {
    prefs_service_->ClearPref(
        brave_account::prefs::kBraveAccountAuthenticationToken);
  } else {
    prefs_service_->SetString(
        brave_account::prefs::kBraveAccountAuthenticationToken, encrypted);
  }
}

std::string EmailAliasesAuth::GetAuthEmail() const {
  return auth_email_;
}

std::string EmailAliasesAuth::CheckAndGetAuthToken() {
  const auto encrypted_token = prefs_service_->GetString(
      brave_account::prefs::kBraveAccountAuthenticationToken);
  if (encrypted_token.empty()) {
    return {};
  }

  std::string token;
  if (!Decrypt(encryptor_, encrypted_token, token)) {
    // Failed to decrypt token -> reset.
    SetAuthToken({});
    return {};
  }
  return token;
}

void EmailAliasesAuth::OnPrefChanged(const std::string& pref_name) {
  if (!notify_) {
    return;
  }

  base::AutoReset reenter(&notify_, false);

  auto auth_email = GetAuthEmail();
  if (auth_email != auth_email_) {
    SetAuthToken({});
    auth_email_ = std::move(auth_email);
  }

  is_authenticated_ = !CheckAndGetAuthToken().empty() && !auth_email_.empty();
  on_changed_.Run();
}

}  // namespace email_aliases
