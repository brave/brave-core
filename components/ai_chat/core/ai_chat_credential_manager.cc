/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/ai_chat_credential_manager.h"

#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/json/values_util.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/brave_domains/service_domains.h"
#include "brave/components/ai_chat/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "net/cookies/cookie_inclusion_status.h"
#include "net/cookies/cookie_util.h"
#include "net/cookies/parsed_cookie.h"
#include "url/url_util.h"

namespace {

const char kLeoSkuHostnamePart[] = "leo";

}  // namespace

namespace ai_chat {

AIChatCredentialManager::AIChatCredentialManager(
    base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
        skus_service_getter,
    PrefService* prefs_service)
    : skus_service_getter_(std::move(skus_service_getter)),
      prefs_service_(prefs_service) {}
AIChatCredentialManager::~AIChatCredentialManager() = default;

void AIChatCredentialManager::GetPremiumStatus(
    ai_chat::mojom::PageHandler::GetPremiumStatusCallback callback) {
  base::Time now = base::Time::Now();
  // First check for a valid credential in the cache.
  const auto& cached_creds_dict =
      prefs_service_->GetDict(ai_chat::prefs::kBraveChatPremiumCredentialCache);
  for (const auto [credential, expires_at_value] : cached_creds_dict) {
    absl::optional<base::Time> expires_at = base::ValueToTime(expires_at_value);
    if (!expires_at) {
      continue;
    }

    if (*expires_at > now) {
      std::move(callback).Run(ai_chat::mojom::PremiumStatus::Active);
      return;
    }
  }

  const std::string leo_sku_domain =
      brave_domains::GetServicesDomain(kLeoSkuHostnamePart);

  // If there aren't any valid in the cache, we must check the CredentialSummary
  // from from the SKU service.
  if (!EnsureMojoConnected()) {
    // This profile can't check skus
    // TODO(petemill): Pass the original profile skus service from
    // the incognito profile.
    std::move(callback).Run(mojom::PremiumStatus::Inactive);
    return;
  }
  DCHECK(skus_service_);
  skus_service_->CredentialSummary(
      leo_sku_domain,
      base::BindOnce(&AIChatCredentialManager::OnCredentialSummary,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     leo_sku_domain));
}

void AIChatCredentialManager::OnCredentialSummary(
    ai_chat::mojom::PageHandler::GetPremiumStatusCallback callback,
    const std::string& domain,
    const std::string& summary_string) {
  std::string summary_string_trimmed;
  base::TrimWhitespaceASCII(summary_string, base::TrimPositions::TRIM_ALL,
                            &summary_string_trimmed);

  if (summary_string_trimmed.empty()) {
    std::move(callback).Run(ai_chat::mojom::PremiumStatus::Inactive);
    return;
  }

  absl::optional<base::Value> records_v = base::JSONReader::Read(
      summary_string, base::JSONParserOptions::JSON_PARSE_RFC);

  if (!records_v || !records_v->is_dict()) {
    std::move(callback).Run(ai_chat::mojom::PremiumStatus::Inactive);
    return;
  }

  const auto& records_dict = records_v->GetDict();
  // Empty dict - "{}" - all credentials are expired or it's a new user.
  if (records_dict.empty()) {
    std::move(callback).Run(ai_chat::mojom::PremiumStatus::Inactive);
    return;
  }

  int remaining_count =
      records_dict.FindInt("remaining_credential_count").value_or(0);
  const std::string* expires_at = records_dict.FindString("expires_at");
  // If the user has no more credentials AND expires_at is empty, then
  // the user is disconnected (needs to refresh). If expires_at is not empty,
  // the user has just run out of credentials, and they need to wait until a
  // refresh is available.
  if (remaining_count == 0 && (!expires_at || expires_at->empty())) {
    std::move(callback).Run(ai_chat::mojom::PremiumStatus::ActiveDisconnected);
    return;
  }

  std::move(callback).Run(ai_chat::mojom::PremiumStatus::Active);
}

void AIChatCredentialManager::FetchPremiumCredential(
    base::OnceCallback<void(absl::optional<CredentialCacheEntry> credential)>
        callback) {
  // Loop through credentials looking for a valid credential and remove it. If
  // there is more than one valid credential, use the one that is expiring
  // soonest. Also, remove any expired credentials as we go.
  ScopedDictPrefUpdate update(prefs_service_,
                              ai_chat::prefs::kBraveChatPremiumCredentialCache);
  base::Value::Dict& dict = update.Get();
  base::Time now = base::Time::Now();
  CredentialCacheEntry valid_credential;
  bool found_valid_credential = false;
  base::Time nearest_expiration = base::Time::Max();

  // Collect keys of credentials to be removed.
  std::vector<std::string> keys_to_erase;

  for (auto it = dict.begin(); it != dict.end(); ++it) {
    base::Value& expires_at_value = it->second;
    absl::optional<base::Time> expires_at = base::ValueToTime(expires_at_value);

    // Remove expired credentials from the cache.
    if (!expires_at || *expires_at < now) {
      keys_to_erase.push_back(it->first);
      continue;
    }

    // Check if this credential is closer to expiration than the current
    // nearest.
    if (*expires_at < nearest_expiration) {
      nearest_expiration = *expires_at;
      valid_credential.credential = it->first;
      valid_credential.expires_at = *expires_at;
      found_valid_credential = true;
    }
  }

  if (found_valid_credential) {
    keys_to_erase.push_back(valid_credential.credential);
  }

  // Erase invalid and nearest credentials.
  for (const auto& key : keys_to_erase) {
    dict.Remove(key);
  }

  // Use credential from the cache if it existed.
  if (found_valid_credential) {
    std::move(callback).Run(valid_credential);
    return;
  }

  // Otherwise, fetch a fresh credential using the SKUs SDK.
  GetPremiumStatus(base::BindOnce(&AIChatCredentialManager::OnGetPremiumStatus,
                                  weak_ptr_factory_.GetWeakPtr(),
                                  std::move(callback)));
}

void AIChatCredentialManager::OnGetPremiumStatus(
    base::OnceCallback<void(absl::optional<CredentialCacheEntry> credential)>
        callback,
    ai_chat::mojom::PremiumStatus status) {
  if (status != ai_chat::mojom::PremiumStatus::Active) {
    std::move(callback).Run(absl::nullopt);
    return;
  }
  const std::string leo_sku_domain =
      brave_domains::GetServicesDomain(kLeoSkuHostnamePart);

  if (!EnsureMojoConnected()) {
    std::move(callback).Run({});
    return;
  }
  DCHECK(skus_service_);
  skus_service_->PrepareCredentialsPresentation(
      leo_sku_domain, "*",
      base::BindOnce(&AIChatCredentialManager::OnPrepareCredentialsPresentation,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     leo_sku_domain));
}

void AIChatCredentialManager::OnPrepareCredentialsPresentation(
    base::OnceCallback<void(absl::optional<CredentialCacheEntry> credential)>
        callback,
    const std::string& domain,
    const std::string& credential_as_cookie) {
  // Credential is returned in cookie format.
  net::CookieInclusionStatus status;
  net::ParsedCookie credential_cookie(credential_as_cookie,
                                      /*block_truncated=*/true, &status);
  if (!credential_cookie.IsValid()) {
    std::move(callback).Run(absl::nullopt);
    return;
  }

  if (!status.IsInclude()) {
    std::move(callback).Run(absl::nullopt);
    return;
  }

  if (!credential_cookie.HasExpires()) {
    std::move(callback).Run(absl::nullopt);
    return;
  }

  const auto time =
      net::cookie_util::ParseCookieExpirationTime(credential_cookie.Expires());
  // Early return when it's already expired.
  if (time < base::Time::Now()) {
    std::move(callback).Run(absl::nullopt);
    return;
  }

  // Credential value received needs to be URL decoded.
  // That leaves us with a Base64 encoded JSON blob which is the credential.
  const std::string encoded_credential = credential_cookie.Value();
  url::RawCanonOutputT<char16_t> unescaped;
  url::DecodeURLEscapeSequences(
      encoded_credential, url::DecodeURLMode::kUTF8OrIsomorphic, &unescaped);
  std::string credential;
  base::UTF16ToUTF8(unescaped.data(), unescaped.length(), &credential);
  if (credential.empty()) {
    // Not purchased.
    std::move(callback).Run(absl::nullopt);
    return;
  }

  CredentialCacheEntry entry;
  entry.credential = credential;
  entry.expires_at = time;
  std::move(callback).Run(entry);
}

void AIChatCredentialManager::PutCredentialInCache(
    CredentialCacheEntry credential) {
  ScopedDictPrefUpdate update(prefs_service_,
                              ai_chat::prefs::kBraveChatPremiumCredentialCache);
  base::Value::Dict& dict = update.Get();
  dict.Set(credential.credential, base::TimeToValue(credential.expires_at));
}

bool AIChatCredentialManager::EnsureMojoConnected() {
  // Bind if not bound yet
  if (!skus_service_) {
    auto pending = skus_service_getter_.Run();
    if (pending.is_valid()) {
      skus_service_.Bind(std::move(pending));
    }
  }
  // Some profiles can't have skus service, so we still might not have one
  if (skus_service_) {
    skus_service_.set_disconnect_handler(
        base::BindOnce(&AIChatCredentialManager::OnMojoConnectionError,
                       weak_ptr_factory_.GetWeakPtr()));
  }
  return !!skus_service_;
}

void AIChatCredentialManager::OnMojoConnectionError() {
  skus_service_.reset();
  EnsureMojoConnected();
}

}  // namespace ai_chat
