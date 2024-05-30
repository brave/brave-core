/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/server_config_loader.h"

#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/location.h"
#include "base/rand_util.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/components/web_discovery/browser/pref_names.h"
#include "brave/components/web_discovery/browser/util.h"
#include "components/prefs/pref_service.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "third_party/zlib/google/compression_utils.h"

namespace web_discovery {

namespace {

constexpr base::TimeDelta kMinReloadInterval = base::Hours(1);
constexpr base::TimeDelta kMaxReloadInterval = base::Hours(4);

constexpr base::TimeDelta kPatternsMaxAge = base::Hours(2);
constexpr base::TimeDelta kPatternsRequestLatestDelay = base::Minutes(3);
constexpr base::TimeDelta kPatternsRequestInitDelay = base::Seconds(15);

constexpr size_t kPatternsMaxFileSize = 128000;

constexpr net::NetworkTrafficAnnotationTag kNetworkTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("wdp_config", R"(
    semantics {
      sender: "Brave Web Discovery Server Configuration Fetch"
      description:
        "Requests server configuration needed to send Web Discovery "
        "measurements to Brave servers."
      trigger:
        "Requests are automatically sent at intervals "
        "while Brave is running."
      data: "Configuration attributes"
      destination: WEBSITE
    }
    policy {
      cookies_allowed: NO
      setting:
        "Users can opt-in or out via brave://settings/search"
    })");

constexpr char kGroupPubKeysFieldName[] = "groupPubKeys";
constexpr char kPubKeysFieldName[] = "pubKeys";
constexpr char kMinVersionFieldName[] = "minVersion";

constexpr char kConfigPathWithFields[] =
    "/config?fields=minVersion,groupPubKeys,pubKeys,sourceMap";
constexpr char kPatternsFilename[] = "wdp_patterns.json";

KeyMap ParseKeys(const base::Value::Dict& encoded_keys) {
  KeyMap map;
  for (const auto [date, key_b64] : encoded_keys) {
    std::vector<uint8_t> decoded_data;
    // Decode to check for valid base64
    if (!base::Base64Decode(key_b64.GetString())) {
      continue;
    }
    map[date] = key_b64.GetString();
  }
  return map;
}

std::optional<std::string> GunzipContents(std::string gzipped_contents) {
  std::string result;
  if (!compression::GzipUncompress(gzipped_contents, &result)) {
    return std::nullopt;
  }
  return result;
}

bool WritePatternsFile(base::FilePath patterns_path, std::string contents) {
  return base::WriteFile(patterns_path, contents);
}

std::optional<std::string> ReadPatternsFile(base::FilePath patterns_path) {
  std::string contents;
  if (!base::ReadFileToStringWithMaxSize(patterns_path, &contents,
                                         kPatternsMaxFileSize)) {
    return std::nullopt;
  }
  return contents;
}

}  // namespace

ServerConfig::ServerConfig() = default;

ServerConfig::~ServerConfig() = default;

ServerConfigLoader::ServerConfigLoader(
    PrefService* local_state,
    base::FilePath user_data_dir,
    network::SharedURLLoaderFactory* shared_url_loader_factory,
    ConfigCallback config_callback,
    PatternsCallback patterns_callback)
    : local_state_(local_state),
      pool_sequenced_task_runner_(
          base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()})),
      shared_url_loader_factory_(shared_url_loader_factory),
      config_callback_(config_callback),
      patterns_callback_(patterns_callback),
      config_backoff_entry_(&kBackoffPolicy),
      patterns_backoff_entry_(&kBackoffPolicy) {
  config_url_ = GURL(GetCollectorHost() + kConfigPathWithFields);
  patterns_url_ = GetPatternsEndpoint();

  patterns_path_ = user_data_dir.Append(kPatternsFilename);
}

ServerConfigLoader::~ServerConfigLoader() = default;

void ServerConfigLoader::Load() {
  // TODO(djandries): create a backoff url loader if one doenst exist
  // but consider how join retries will work before you do. we might want to
  // generate gsk beforehand... in which case it might make sense to add a
  // repeating callback for generating state + network requests
  if (config_url_loader_) {
    // Another request is in progress
    return;
  }
  auto resource_request = CreateResourceRequest(config_url_);

  config_url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), kNetworkTrafficAnnotation);

  config_url_loader_->DownloadToString(
      shared_url_loader_factory_.get(),
      base::BindOnce(&ServerConfigLoader::OnConfigResponse,
                     base::Unretained(this)),
      kMaxResponseSize);
}

void ServerConfigLoader::OnConfigResponse(
    std::optional<std::string> response_body) {
  base::Time update_time = base::Time::Now();
  bool result = ProcessConfigResponse(response_body);

  config_backoff_entry_.InformOfRequest(result);

  if (!result) {
    update_time += config_backoff_entry_.GetTimeUntilRelease();
  } else {
    update_time += base::RandTimeDelta(kMinReloadInterval, kMaxReloadInterval);

    SchedulePatternsRequest();
  }

  config_update_timer_.Start(
      FROM_HERE, update_time,
      base::BindOnce(&ServerConfigLoader::Load, base::Unretained(this)));
}

bool ServerConfigLoader::ProcessConfigResponse(
    const std::optional<std::string>& response_body) {
  auto* response_info = config_url_loader_->ResponseInfo();
  if (!response_body || !response_info ||
      response_info->headers->response_code() != 200) {
    VLOG(1) << "Failed to fetch server config";
    return false;
  }

  auto parsed_json = base::JSONReader::ReadAndReturnValueWithError(
      *response_body, base::JSON_PARSE_RFC);

  if (!parsed_json.has_value()) {
    VLOG(1) << "Failed to parse server config json";
    return false;
  }

  const auto* root = parsed_json.value().GetIfDict();
  if (!root) {
    VLOG(1) << "Failed to parse server config: not a dict";
    return false;
  }

  const auto min_version = root->FindInt(kMinVersionFieldName);
  if (min_version && *min_version > kCurrentVersion) {
    VLOG(1) << "Server minimum version is higher than current version, failing";
    return false;
  }

  auto config = std::make_unique<ServerConfig>();

  const auto* group_pub_keys = root->FindDict(kGroupPubKeysFieldName);
  if (!group_pub_keys) {
    VLOG(1) << "Failed to retrieve groupPubKeys from server config";
    return false;
  }
  const auto* pub_keys = root->FindDict(kPubKeysFieldName);
  if (!pub_keys) {
    VLOG(1) << "Failed to retrieve pubKeys from server config";
    return false;
  }

  config->group_pub_keys = ParseKeys(*group_pub_keys);
  config->pub_keys = ParseKeys(*pub_keys);

  config_callback_.Run(std::move(config));
  return true;
}

void ServerConfigLoader::LoadStoredPatterns() {
  pool_sequenced_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&ReadPatternsFile, patterns_path_),
      base::BindOnce(&ServerConfigLoader::OnPatternsFileLoaded,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ServerConfigLoader::OnPatternsFileLoaded(
    std::optional<std::string> patterns_json) {
  if (!patterns_json) {
    VLOG(1) << "Failed to load local patterns file";
    local_state_->ClearPref(kPatternsRetrievalTime);
    SchedulePatternsRequest();
    return;
  }
  auto parsed_patterns = ParsePatterns(*patterns_json);
  if (!parsed_patterns) {
    local_state_->ClearPref(kPatternsRetrievalTime);
    SchedulePatternsRequest();
    return;
  }
  patterns_callback_.Run(std::move(parsed_patterns));
}

void ServerConfigLoader::SchedulePatternsRequest() {
  base::Time update_time = base::Time::Now();
  base::TimeDelta time_since_last_retrieval =
      base::Time::Now() - local_state_->GetTime(kPatternsRetrievalTime);
  if (time_since_last_retrieval >= kPatternsMaxAge) {
    update_time += kPatternsRequestInitDelay;
  } else {
    if (!patterns_first_request_made_) {
      LoadStoredPatterns();
    }
    update_time += kPatternsMaxAge - time_since_last_retrieval +
                   base::RandTimeDelta({}, kPatternsRequestLatestDelay);
  }
  patterns_first_request_made_ = true;
  patterns_update_timer_.Start(
      FROM_HERE, update_time,
      base::BindOnce(&ServerConfigLoader::RequestPatterns,
                     base::Unretained(this)));
}

void ServerConfigLoader::RequestPatterns() {
  if (patterns_url_loader_) {
    return;
  }
  auto resource_request = CreateResourceRequest(patterns_url_);

  patterns_url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), kNetworkTrafficAnnotation);

  patterns_url_loader_->DownloadToString(
      shared_url_loader_factory_.get(),
      base::BindOnce(&ServerConfigLoader::OnPatternsResponse,
                     base::Unretained(this)),
      kMaxResponseSize);
}

void ServerConfigLoader::HandlePatternsStatus(bool result) {
  patterns_url_loader_ = nullptr;
  patterns_backoff_entry_.InformOfRequest(result);

  if (result) {
    SchedulePatternsRequest();
    return;
  }

  patterns_update_timer_.Start(
      FROM_HERE,
      base::Time::Now() + patterns_backoff_entry_.GetTimeUntilRelease(),
      base::BindOnce(&ServerConfigLoader::RequestPatterns,
                     base::Unretained(this)));
}

void ServerConfigLoader::OnPatternsResponse(
    std::optional<std::string> response_body) {
  auto* response_info = config_url_loader_->ResponseInfo();
  if (!response_body || !response_info ||
      response_info->headers->response_code() != 200) {
    VLOG(1) << "Failed to retrieve patterns file";
    HandlePatternsStatus(false);
    return;
  }
  pool_sequenced_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&GunzipContents, *response_body),
      base::BindOnce(&ServerConfigLoader::OnPatternsGunzip,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ServerConfigLoader::OnPatternsGunzip(
    std::optional<std::string> patterns_json) {
  if (!patterns_json) {
    VLOG(1) << "Failed to decompress patterns file";
    HandlePatternsStatus(false);
    return;
  }
  auto parsed_patterns = ParsePatterns(*patterns_json);
  if (!parsed_patterns) {
    HandlePatternsStatus(false);
    return;
  }
  pool_sequenced_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&WritePatternsFile, patterns_path_, *patterns_json),
      base::BindOnce(&ServerConfigLoader::OnPatternsWritten,
                     weak_ptr_factory_.GetWeakPtr(),
                     std::move(parsed_patterns)));
}

void ServerConfigLoader::OnPatternsWritten(
    std::unique_ptr<PatternsGroup> parsed_group,
    bool result) {
  if (!result) {
    VLOG(1) << "Failed to write patterns file";
    HandlePatternsStatus(false);
    return;
  }
  local_state_->SetTime(kPatternsRetrievalTime, base::Time::Now());
  HandlePatternsStatus(true);
  patterns_callback_.Run(std::move(parsed_group));
}

}  // namespace web_discovery
