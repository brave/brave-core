/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/browser/ai_chat_credential_manager.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/ai_chat/browser/constants.h"
#include "net/cookies/cookie_inclusion_status.h"
#include "net/cookies/cookie_util.h"
#include "net/cookies/parsed_cookie.h"
#include "url/url_util.h"

namespace ai_chat {

AIChatCredentialManager::AIChatCredentialManager(
    base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
        skus_service_getter)
    : skus_service_getter_(std::move(skus_service_getter)) {}
AIChatCredentialManager::~AIChatCredentialManager() = default;

void AIChatCredentialManager::UserHasValidPremiumCredential(
    UserHasValidPremiumCredentialCallback callback) {
  EnsureMojoConnected();
  skus_service_->CredentialSummary(
      kLeoSkuDomain,
      base::BindOnce(&AIChatCredentialManager::OnCredentialSummary,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     kLeoSkuDomain));
}

void AIChatCredentialManager::OnCredentialSummary(
    UserHasValidPremiumCredentialCallback callback,
    const std::string& domain,
    const std::string& summary_string) {
  std::string summary_string_trimmed;
  base::TrimWhitespaceASCII(summary_string, base::TrimPositions::TRIM_ALL,
                            &summary_string_trimmed);
  if (summary_string_trimmed.length() == 0) {
    // no credential found; person needs to login
    std::move(callback).Run(false);
    return;
  }

  absl::optional<base::Value> records_v = base::JSONReader::Read(
      summary_string, base::JSONParserOptions::JSON_PARSE_RFC);

  // Early return when summary is invalid or it's empty dict.
  if (!records_v || !records_v->is_dict()) {
    std::move(callback).Run(false);
    return;
  }

  // Empty dict - clean user.
  if (records_v->GetDict().empty()) {
    std::move(callback).Run(false);
    return;
  }

  // For now, if there are any records in CredentialSummary, then I'm assuming
  // it's active, even if active is literally false.
  //
  // const bool active =
  // (*records_v).GetDict().FindBool("active").value_or(false); if (!active) {
  //   std::move(callback).Run(false);
  //   return;
  // }

  std::move(callback).Run(true);
}

void AIChatCredentialManager::FetchPremiumCredential(
    FetchPremiumCredentialCallback callback) {
  UserHasValidPremiumCredential(
      base::BindOnce(&AIChatCredentialManager::OnUserHasValidPremiumCredential,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AIChatCredentialManager::OnUserHasValidPremiumCredential(
    FetchPremiumCredentialCallback callback,
    bool result) {
  if (!result) {
    std::move(callback).Run(absl::nullopt);
    return;
  }

  EnsureMojoConnected();
  skus_service_->PrepareCredentialsPresentation(
      kLeoSkuDomain, "*",
      base::BindOnce(&AIChatCredentialManager::OnPrepareCredentialsPresentation,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     kLeoSkuDomain));
}

void AIChatCredentialManager::OnPrepareCredentialsPresentation(
    FetchPremiumCredentialCallback callback,
    const std::string& domain,
    const std::string& credential_as_cookie) {
  // Credential is returned in cookie format.
  net::CookieInclusionStatus status;
  net::ParsedCookie credential_cookie(credential_as_cookie, &status);
  if (!credential_cookie.IsValid()) {
    VLOG(1) << __func__ << " : FAILED credential_cookie.IsValid";
    std::move(callback).Run(absl::nullopt);
    return;
  }

  if (!status.IsInclude()) {
    VLOG(1) << __func__ << " : FAILED status.IsInclude";
    std::move(callback).Run(absl::nullopt);
    return;
  }

  if (!credential_cookie.HasExpires()) {
    std::move(callback).Run(absl::nullopt);
    return;
  }

  // Credential value received needs to be URL decoded.
  // That leaves us with a Base64 encoded JSON blob which is the credential.
  const std::string encoded_credential = credential_cookie.Value();
  const auto time =
      net::cookie_util::ParseCookieExpirationTime(credential_cookie.Expires());
  url::RawCanonOutputT<char16_t> unescaped;
  url::DecodeURLEscapeSequences(
      encoded_credential.data(), encoded_credential.size(),
      url::DecodeURLMode::kUTF8OrIsomorphic, &unescaped);
  std::string credential;
  base::UTF16ToUTF8(unescaped.data(), unescaped.length(), &credential);
  if (credential.empty()) {
    // Not purchased.
    std::move(callback).Run(absl::nullopt);
    return;
  }

  // Early return when it's already expired.
  if (time < base::Time::Now()) {
    std::move(callback).Run(absl::nullopt);
    return;
  }

  std::move(callback).Run(credential);
}

void AIChatCredentialManager::EnsureMojoConnected() {
  if (!skus_service_) {
    auto pending = skus_service_getter_.Run();
    skus_service_.Bind(std::move(pending));
  }
  DCHECK(skus_service_);
  skus_service_.set_disconnect_handler(
      base::BindOnce(&AIChatCredentialManager::OnMojoConnectionError,
                     weak_ptr_factory_.GetWeakPtr()));
}

void AIChatCredentialManager::OnMojoConnectionError() {
  skus_service_.reset();
  EnsureMojoConnected();
}

}  // namespace ai_chat
