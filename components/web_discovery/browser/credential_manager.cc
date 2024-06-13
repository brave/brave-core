/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/credential_manager.h"

#include <utility>

#include "base/base64.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "brave/components/web_discovery/browser/anonymous_credentials/rs/src/lib.rs.h"
#include "brave/components/web_discovery/browser/pref_names.h"
#include "brave/components/web_discovery/browser/util.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "crypto/sha2.h"
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

std::optional<GenerateJoinRequestResult> GenerateJoinRequest(
    anonymous_credentials::CredentialManager* anonymous_credential_manager,
    EVPKeyPtr* rsa_private_key,
    std::string pre_challenge) {
  base::span<uint8_t> pre_challenge_span(
      reinterpret_cast<uint8_t*>(pre_challenge.data()), pre_challenge.size());
  auto challenge = crypto::SHA256Hash(pre_challenge_span);

  auto join_request = anonymous_credential_manager->start_join(
      rust::Slice<const uint8_t>(challenge.data(), challenge.size()));

  auto signature = RSASign(*rsa_private_key, join_request.join_request);

  if (!signature) {
    VLOG(1) << "RSA signature failed";
    return std::nullopt;
  }

  return GenerateJoinRequestResult{.start_join_result = join_request,
                                   .signature = *signature};
}

std::optional<std::string> FinishJoin(
    anonymous_credentials::CredentialManager* anonymous_credential_manager,
    std::string date,
    std::vector<const uint8_t> group_pub_key,
    std::vector<const uint8_t> gsk,
    std::vector<const uint8_t> join_resp_bytes) {
  auto finish_res = anonymous_credential_manager->finish_join(
      rust::Slice(group_pub_key.data(), group_pub_key.size()),
      rust::Slice(gsk.data(), gsk.size()),
      rust::Slice(join_resp_bytes.data(), join_resp_bytes.size()));
  if (!finish_res.error_message.empty()) {
    VLOG(1) << "Failed to finish credential join for " << date << ": "
            << finish_res.error_message.c_str();
    return std::nullopt;
  }
  return base::Base64Encode(finish_res.data);
}

std::optional<std::vector<const uint8_t>> PerformSign(
    anonymous_credentials::CredentialManager* anonymous_credential_manager,
    std::vector<const uint8_t> msg,
    std::vector<const uint8_t> basename,
    std::optional<std::vector<uint8_t>> gsk_bytes,
    std::optional<std::vector<uint8_t>> credential_bytes) {
  if (gsk_bytes && credential_bytes) {
    auto set_res = anonymous_credential_manager->set_gsk_and_credentials(
        rust::Slice(reinterpret_cast<const uint8_t*>(gsk_bytes->data()),
                    gsk_bytes->size()),
        rust::Slice(reinterpret_cast<const uint8_t*>(credential_bytes->data()),
                    credential_bytes->size()));
    if (!set_res.error_message.empty()) {
      VLOG(1) << "Failed to sign due to credential set failure: "
              << set_res.error_message.c_str();
      return std::nullopt;
    }
  }
  auto sig_res = anonymous_credential_manager->sign(
      rust::Slice(msg.data(), msg.size()),
      rust::Slice(basename.data(), basename.size()));
  if (!sig_res.error_message.empty()) {
    VLOG(1) << "Failed to sign: " << sig_res.error_message.c_str();
    return std::nullopt;
  }
  return std::vector<const uint8_t>(sig_res.data.begin(), sig_res.data.end());
}

}  // namespace

CredentialManager::CredentialManager(
    PrefService* profile_prefs,
    network::SharedURLLoaderFactory* shared_url_loader_factory,
    std::unique_ptr<ServerConfig>* last_loaded_server_config)
    : profile_prefs_(profile_prefs),
      shared_url_loader_factory_(shared_url_loader_factory),
      last_loaded_server_config_(last_loaded_server_config),
      join_url_(GetDirectHPNHost() + kJoinPath),
      backoff_entry_(&kBackoffPolicy),
      pool_sequenced_task_runner_(
          base::ThreadPool::CreateSequencedTaskRunner({})),
      anonymous_credential_manager_(
          new rust::Box(anonymous_credentials::new_credential_manager()),
          base::OnTaskRunnerDeleter(pool_sequenced_task_runner_)),
      rsa_private_key_(new EVPKeyPtr(),
                       base::OnTaskRunnerDeleter(pool_sequenced_task_runner_)) {
}

CredentialManager::~CredentialManager() = default;

bool CredentialManager::LoadRSAKey() {
  std::string private_key_b64 =
      profile_prefs_->GetString(kCredentialRSAPrivateKey);
  rsa_public_key_b64_ = profile_prefs_->GetString(kCredentialRSAPublicKey);

  if (private_key_b64.empty() || !rsa_public_key_b64_->empty()) {
    rsa_public_key_b64_ = std::nullopt;
    return true;
  }

  *rsa_private_key_ = ImportRSAKeyPair(private_key_b64);

  if (!rsa_private_key_) {
    VLOG(1) << "Failed to decode stored RSA key";
    return false;
  }

  return true;
}

void CredentialManager::OnNewRSAKey(std::unique_ptr<RSAKeyInfo> key_info) {
  if (!key_info) {
    VLOG(1) << "RSA key generation failed";
    return;
  }

  *rsa_private_key_ = std::move(key_info->key_pair);
  rsa_public_key_b64_ = key_info->public_key_b64;

  profile_prefs_->SetString(kCredentialRSAPrivateKey,
                            key_info->private_key_b64);
  profile_prefs_->SetString(kCredentialRSAPublicKey, *rsa_public_key_b64_);

  JoinGroups();
}

void CredentialManager::JoinGroups() {
  if (!*last_loaded_server_config_) {
    return;
  }
  auto today_date = FormatServerDate(base::Time::Now().UTCMidnight());
  const auto& anon_creds_dict =
      profile_prefs_->GetDict(kAnonymousCredentialsDict);
  for (const auto& [date, group_pub_key_b64] :
       (*last_loaded_server_config_)->group_pub_keys) {
    if (date < today_date || join_url_loaders_.contains(date) ||
        anon_creds_dict.contains(date)) {
      continue;
    }

    if (*rsa_private_key_ == nullptr) {
      if (!LoadRSAKey()) {
        return;
      }
      if (*rsa_private_key_ == nullptr) {
        pool_sequenced_task_runner_->PostTaskAndReplyWithResult(
            FROM_HERE, base::BindOnce(&GenerateRSAKeyPair),
            base::BindOnce(&CredentialManager::OnNewRSAKey,
                           weak_ptr_factory_.GetWeakPtr()));
        return;
      }
    }

    StartJoinGroup(date, group_pub_key_b64);
  }
}

void CredentialManager::StartJoinGroup(const std::string& date,
                                       const std::string& group_pub_key_b64) {
  auto group_pub_key = base::Base64Decode(group_pub_key_b64);
  if (!group_pub_key) {
    VLOG(1) << "Failed to decode group public key for " << date;
    return;
  }
  std::vector<const uint8_t> group_pub_key_const(group_pub_key->begin(),
                                                 group_pub_key->end());

  auto challenge_elements = base::Value::List::with_capacity(2);
  challenge_elements.Append(*rsa_public_key_b64_);
  challenge_elements.Append(group_pub_key_b64);

  std::string pre_challenge;
  base::JSONWriter::Write(challenge_elements, &pre_challenge);

  pool_sequenced_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&GenerateJoinRequest, &**anonymous_credential_manager_,
                     rsa_private_key_.get(), pre_challenge),
      base::BindOnce(&CredentialManager::OnJoinRequestReady,
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

  pool_sequenced_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&FinishJoin, &**anonymous_credential_manager_, date,
                     group_pub_key, gsk, join_resp_bytes_const),
      base::BindOnce(&CredentialManager::OnCredentialsReady,
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

bool CredentialManager::Sign(std::vector<const uint8_t> msg,
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
      return false;
    }
    auto* gsk_b64 = today_cred_dict->FindString(kGSKDictKey);
    auto* credential_b64 = today_cred_dict->FindString(kCredentialDictKey);
    if (!gsk_b64 || !credential_b64) {
      VLOG(1) << "Failed to sign due to unavailability of gsk/credential";
      return false;
    }
    gsk_bytes = base::Base64Decode(*gsk_b64);
    credential_bytes = base::Base64Decode(*credential_b64);
    if (!gsk_bytes || !credential_bytes) {
      VLOG(1) << "Failed to sign due to bad gsk/credential base64";
      return false;
    }
  }

  pool_sequenced_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&PerformSign, &**anonymous_credential_manager_, msg,
                     basename, gsk_bytes, credential_bytes),
      base::BindOnce(&CredentialManager::OnSignResult,
                     weak_ptr_factory_.GetWeakPtr(), today_date,
                     std::move(callback)));
  return true;
}

void CredentialManager::OnSignResult(
    std::string credential_date,
    SignCallback callback,
    std::optional<std::vector<const uint8_t>> signed_message) {
  loaded_credential_date_ = credential_date;
  std::move(callback).Run(signed_message);
}

}  // namespace web_discovery
