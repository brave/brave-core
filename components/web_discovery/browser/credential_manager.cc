/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/credential_manager.h"

#include <utility>

#include "base/base64.h"
#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "brave/components/web_discovery/browser/pref_names.h"
#include "brave/components/web_discovery/browser/rsa.h"
#include "brave/components/web_discovery/browser/util.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace web_discovery {

namespace {

constexpr char kJoinPath[] = "/join";
constexpr char kJoinContentType[] = "application/json";

constexpr char kJoinDateField[] = "ts";
constexpr char kJoinMessageField[] = "joinMsg";
constexpr char kJoinRSAPublicKeyField[] = "pk";
constexpr char kJoinRSASignatureField[] = "sig";
constexpr char kJoinResponseField[] = "joinResponse";

constexpr char kGSKDictKey[] = "gsk";
constexpr char kCredentialDictKey[] = "credential";

constexpr net::NetworkTrafficAnnotationTag kJoinNetworkTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("wdp_join", R"(
    semantics {
      sender: "Brave Web Discovery HPNv2 Join"
      description:
        "Retrieves anonymous credentials in order to sign Web Discovery
         measurements sent via the HumanWeb Proxy Network."
      trigger:
        "Requests are automatically sent on daily intervals "
        "while Brave is running."
      data: "Configuration attributes"
      destination: WEBSITE
    }
    policy {
      cookies_allowed: NO
      setting:
        "Users can opt-in or out via brave://settings/search"
    })");

}  // namespace

CredentialManager::CredentialManager(
    PrefService* profile_prefs,
    network::SharedURLLoaderFactory* shared_url_loader_factory,
    const ServerConfigLoader* server_config_loader)
    : profile_prefs_(profile_prefs),
      shared_url_loader_factory_(shared_url_loader_factory),
      server_config_loader_(server_config_loader),
      join_url_(GetDirectHPNHost() + kJoinPath),
      backoff_entry_(&kBackoffPolicy),
      background_task_runner_(base::ThreadPool::CreateSequencedTaskRunner({})),
      background_credential_helper_(background_task_runner_) {}

CredentialManager::~CredentialManager() = default;

bool CredentialManager::LoadRSAKey() {
  std::string private_key_b64 =
      profile_prefs_->GetString(kCredentialRSAPrivateKey);

  if (private_key_b64.empty()) {
    return true;
  }

  auto key_info = ImportRSAKeyPair(private_key_b64);
  if (!key_info) {
    VLOG(1) << "Failed to import stored RSA key";
    return false;
  }

  CHECK(key_info->key_pair);
  rsa_public_key_b64_ = std::move(key_info->public_key_b64);
  background_credential_helper_
      .AsyncCall(&BackgroundCredentialHelper::SetRSAKey)
      .WithArgs(std::move(key_info->key_pair));

  return true;
}

void CredentialManager::OnNewRSAKey(std::unique_ptr<RSAKeyInfo> key_info) {
  if (!key_info) {
    VLOG(1) << "RSA key generation failed";
    return;
  }

  rsa_public_key_b64_ = key_info->public_key_b64;
  CHECK(key_info->key_pair && key_info->private_key_b64);

  profile_prefs_->SetString(kCredentialRSAPrivateKey,
                            *key_info->private_key_b64);

  JoinGroups();
}

void CredentialManager::JoinGroups() {
  const auto& server_config = server_config_loader_->GetLastServerConfig();
  auto today_date = FormatServerDate(base::Time::Now().UTCMidnight());
  const auto& anon_creds_dict =
      profile_prefs_->GetDict(kAnonymousCredentialsDict);
  for (const auto& [date, group_pub_key] : server_config.group_pub_keys) {
    if (date < today_date || join_url_loaders_.contains(date) ||
        anon_creds_dict.contains(date)) {
      continue;
    }

    if (!rsa_public_key_b64_) {
      if (!LoadRSAKey()) {
        return;
      }
      if (!rsa_public_key_b64_) {
        background_credential_helper_
            .AsyncCall(&BackgroundCredentialHelper::GenerateRSAKey)
            .Then(base::BindOnce(&CredentialManager::OnNewRSAKey,
                                 weak_ptr_factory_.GetWeakPtr()));
        return;
      }
    }

    StartJoinGroup(date, group_pub_key);
  }
}

void CredentialManager::StartJoinGroup(
    const std::string& date,
    const std::vector<uint8_t>& group_pub_key) {
  std::vector<const uint8_t> group_pub_key_const(group_pub_key.begin(),
                                                 group_pub_key.end());

  auto challenge_elements = base::Value::List::with_capacity(2);
  challenge_elements.Append(*rsa_public_key_b64_);
  challenge_elements.Append(base::Base64Encode(group_pub_key));

  std::string pre_challenge;
  base::JSONWriter::Write(challenge_elements, &pre_challenge);

  background_credential_helper_
      .AsyncCall(&BackgroundCredentialHelper::GenerateJoinRequest)
      .WithArgs(pre_challenge)
      .Then(base::BindOnce(&CredentialManager::OnJoinRequestReady,
                           weak_ptr_factory_.GetWeakPtr(), date,
                           group_pub_key_const));
}

void CredentialManager::OnJoinRequestReady(
    std::string date,
    std::vector<const uint8_t> group_pub_key,
    std::optional<GenerateJoinRequestResult> generate_join_result) {
  if (!generate_join_result) {
    return;
  }
  base::Value::Dict body_fields;

  body_fields.Set(kJoinDateField, date);
  body_fields.Set(
      kJoinMessageField,
      base::Base64Encode(generate_join_result->start_join_result.join_request));
  body_fields.Set(kJoinRSAPublicKeyField, *rsa_public_key_b64_);
  body_fields.Set(kJoinRSASignatureField, generate_join_result->signature);

  std::string json_body;
  if (!base::JSONWriter::Write(body_fields, &json_body)) {
    VLOG(1) << "Join body serialization failed";
    return;
  }

  auto gsk = std::vector<const uint8_t>(
      generate_join_result->start_join_result.gsk.begin(),
      generate_join_result->start_join_result.gsk.end());

  auto resource_request = CreateResourceRequest(join_url_);
  resource_request->headers.SetHeader(kVersionHeader,
                                      base::NumberToString(kCurrentVersion));
  resource_request->method = net::HttpRequestHeaders::kPostMethod;

  join_url_loaders_[date] = network::SimpleURLLoader::Create(
      std::move(resource_request), kJoinNetworkTrafficAnnotation);
  auto& url_loader = join_url_loaders_[date];

  url_loader->AttachStringForUpload(json_body, kJoinContentType);

  url_loader->DownloadToString(
      shared_url_loader_factory_.get(),
      base::BindOnce(&CredentialManager::OnJoinResponse, base::Unretained(this),
                     date, group_pub_key, gsk),
      kMaxResponseSize);
}

void CredentialManager::OnJoinResponse(
    std::string date,
    std::vector<const uint8_t> group_pub_key,
    std::vector<const uint8_t> gsk,
    std::optional<std::string> response_body) {
  bool result = ProcessJoinResponse(date, group_pub_key, gsk, response_body);
  if (!result) {
    HandleJoinResponseStatus(date, result);
  }
}

void CredentialManager::HandleJoinResponseStatus(const std::string& date,
                                                 bool result) {
  join_url_loaders_.erase(date);
  // TODO(djandries): what if the last request succeeds and the other requests
  // fail? fix
  if (join_url_loaders_.empty()) {
    backoff_entry_.InformOfRequest(result);

    if (!result) {
      retry_timer_.Start(
          FROM_HERE, base::Time::Now() + backoff_entry_.GetTimeUntilRelease(),
          base::BindOnce(&CredentialManager::JoinGroups,
                         base::Unretained(this)));
    }
  }
}

bool CredentialManager::ProcessJoinResponse(
    const std::string& date,
    const std::vector<const uint8_t>& group_pub_key,
    const std::vector<const uint8_t>& gsk,
    const std::optional<std::string>& response_body) {
  CHECK(join_url_loaders_[date]);
  auto& url_loader = join_url_loaders_[date];
  auto* response_info = url_loader->ResponseInfo();
  if (!response_body || !response_info ||
      response_info->headers->response_code() != 200) {
    VLOG(1) << "Failed to fetch credentials for " << date;
    return false;
  }

  auto parsed_json = base::JSONReader::ReadAndReturnValueWithError(
      *response_body, base::JSON_PARSE_RFC);

  if (!parsed_json.has_value()) {
    VLOG(1) << "Failed to parse join response json";
    return false;
  }

  const auto* root = parsed_json.value().GetIfDict();
  if (!root) {
    VLOG(1) << "Failed to parse join response json: not a dict";
    return false;
  }

  const auto* join_resp = root->FindString(kJoinResponseField);
  if (!join_resp) {
    VLOG(1) << "Failed to find content in join response json";
    return false;
  }

  auto join_resp_bytes = base::Base64Decode(*join_resp);
  if (!join_resp_bytes) {
    VLOG(1) << "Failed to decode join response base64";
    return false;
  }
  std::vector<const uint8_t> join_resp_bytes_const(join_resp_bytes->begin(),
                                                   join_resp_bytes->end());

  background_credential_helper_
      .AsyncCall(&BackgroundCredentialHelper::FinishJoin)
      .WithArgs(date, group_pub_key, gsk, join_resp_bytes_const)
      .Then(base::BindOnce(&CredentialManager::OnCredentialsReady,
                           weak_ptr_factory_.GetWeakPtr(), date, gsk));
  return true;
}

void CredentialManager::OnCredentialsReady(
    std::string date,
    std::vector<const uint8_t> gsk,
    std::optional<std::string> credentials) {
  if (!credentials) {
    HandleJoinResponseStatus(date, false);
    return;
  }
  ScopedDictPrefUpdate update(profile_prefs_, kAnonymousCredentialsDict);
  auto* date_dict = update->EnsureDict(date);
  date_dict->Set(kGSKDictKey, base::Base64Encode(gsk));
  date_dict->Set(kCredentialDictKey, *credentials);
  HandleJoinResponseStatus(date, true);
}

bool CredentialManager::CredentialExistsForToday() {
  return profile_prefs_->GetDict(kAnonymousCredentialsDict)
      .contains(FormatServerDate(base::Time::Now()));
}

void CredentialManager::Sign(std::vector<const uint8_t> msg,
                             std::vector<const uint8_t> basename,
                             SignCallback callback) {
  auto today_date = FormatServerDate(base::Time::Now().UTCMidnight());
  const auto& anon_creds_dict =
      profile_prefs_->GetDict(kAnonymousCredentialsDict);
  std::optional<std::vector<uint8_t>> gsk_bytes;
  std::optional<std::vector<uint8_t>> credential_bytes;
  if (!loaded_credential_date_ || loaded_credential_date_ != today_date) {
    auto* today_cred_dict = anon_creds_dict.FindDict(today_date);
    if (!today_cred_dict) {
      VLOG(1) << "Failed to sign due to unavailability of credentials";
      std::move(callback).Run(std::nullopt);
      return;
    }
    auto* gsk_b64 = today_cred_dict->FindString(kGSKDictKey);
    auto* credential_b64 = today_cred_dict->FindString(kCredentialDictKey);
    if (!gsk_b64 || !credential_b64) {
      VLOG(1) << "Failed to sign due to unavailability of gsk/credential";
      std::move(callback).Run(std::nullopt);
      return;
    }
    gsk_bytes = base::Base64Decode(*gsk_b64);
    credential_bytes = base::Base64Decode(*credential_b64);
    if (!gsk_bytes || !credential_bytes) {
      VLOG(1) << "Failed to sign due to bad gsk/credential base64";
      std::move(callback).Run(std::nullopt);
      return;
    }
  }

  background_credential_helper_
      .AsyncCall(&BackgroundCredentialHelper::PerformSign)
      .WithArgs(msg, basename, gsk_bytes, credential_bytes)
      .Then(base::BindOnce(&CredentialManager::OnSignResult,
                           weak_ptr_factory_.GetWeakPtr(), today_date,
                           std::move(callback)));
}

void CredentialManager::OnSignResult(
    std::string credential_date,
    SignCallback callback,
    std::optional<std::vector<const uint8_t>> signed_message) {
  loaded_credential_date_ = credential_date;
  std::move(callback).Run(signed_message);
}

void CredentialManager::UseFixedSeedForTesting() {
  background_credential_helper_.AsyncCall(
      &BackgroundCredentialHelper::UseFixedSeedForTesting);
}

}  // namespace web_discovery
