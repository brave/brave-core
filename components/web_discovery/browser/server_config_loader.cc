/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/server_config_loader.h"

#include <utility>

#include "base/barrier_callback.h"
#include "base/base64.h"
#include "base/check.h"
#include "base/check_op.h"
#include "base/containers/fixed_flat_set.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/rand_util.h"
#include "base/task/thread_pool.h"
#include "base/threading/thread_restrictions.h"
#include "base/values.h"
#include "brave/components/web_discovery/browser/pref_names.h"
#include "brave/components/web_discovery/browser/util.h"
#include "brave/components/web_discovery/common/features.h"
#include "components/prefs/pref_service.h"
#include "net/http/http_status_code.h"
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
constexpr char kKeysFieldName[] = "keys";
constexpr char kLimitFieldName[] = "limit";
constexpr char kPeriodFieldName[] = "period";
constexpr char kSourceMapFieldName[] = "sourceMap";
constexpr char kSourceMapActionsFieldName[] = "actions";
constexpr char kLocationFieldName[] = "location";

constexpr char kCollectorConfigPathWithFields[] =
    "/config?fields=minVersion,groupPubKeys,pubKeys,sourceMap";
constexpr char kQuorumConfigPath[] = "/config";
constexpr char kPatternsFilename[] = "wdp_patterns.json";
constexpr char kV2PatternsFilename[] = "wdp_patterns_v2.json";

constexpr char kOmittedLocationValue[] = "--";
constexpr auto kAllowedReportLocations =
    base::MakeFixedFlatSet<std::string_view>(
        {"ar", "at", "au", "be", "br", "ca", "ch", "cn", "cz", "de",
         "dk", "es", "fi", "fr", "gb", "gr", "hu", "in", "it", "jp",
         "mx", "nl", "no", "pl", "ro", "ru", "se", "ua", "us"});

KeyMap ParseKeys(const base::Value::Dict& encoded_keys) {
  KeyMap map;
  for (const auto [date, key_b64] : encoded_keys) {
    auto decoded_data = base::Base64Decode(key_b64.GetString());
    if (!decoded_data) {
      continue;
    }
    map[date] = std::move(*decoded_data);
  }
  return map;
}

base::flat_map<std::string, std::unique_ptr<SourceMapActionConfig>>
ParseSourceMapActionConfigs(const base::Value::Dict& configs_dict) {
  base::flat_map<std::string, std::unique_ptr<SourceMapActionConfig>> map;
  for (const auto [action, config_dict_val] : configs_dict) {
    auto* config_dict = config_dict_val.GetIfDict();
    if (!config_dict) {
      continue;
    }
    auto& action_config = map[action];
    if (!action_config) {
      action_config = std::make_unique<SourceMapActionConfig>();
    }
    auto* keys_list = config_dict->FindList(kKeysFieldName);
    if (keys_list) {
      for (auto& key_val : *keys_list) {
        if (key_val.is_string()) {
          action_config->keys.push_back(std::move(key_val.GetString()));
        }
      }
    }
    auto limit = config_dict->FindInt(kLimitFieldName);
    auto period = config_dict->FindInt(kPeriodFieldName);

    action_config->limit = limit && limit > 0 ? *limit : 1;
    action_config->period = period && period > 0 ? *period : 24;
  }
  return map;
}

ParsedPatternsVariant NullPatternsVariant() {
  if (features::ShouldUseV2Patterns()) {
    return ParsedPatternsVariant(std::unique_ptr<V2PatternsGroup>{});
  } else {
    return ParsedPatternsVariant(std::unique_ptr<PatternsGroup>{});
  }
}

ParsedPatternsVariant ParsePatternsContent(const std::string& content) {
  if (features::ShouldUseV2Patterns()) {
    auto v2_patterns = ParseV2Patterns(content);
    if (!v2_patterns) {
      return NullPatternsVariant();
    }
    return std::move(v2_patterns);
  } else {
    auto v1_patterns = ParsePatterns(content);
    if (!v1_patterns) {
      return NullPatternsVariant();
    }
    return std::move(v1_patterns);
  }
}

ParsedPatternsVariant ParseAndWritePatternsFile(base::FilePath patterns_path,
                                                std::string gzipped_contents) {
  std::string uncompressed_contents;
  if (!compression::GzipUncompress(gzipped_contents, &uncompressed_contents)) {
    VLOG(1) << "Failed to uncompress patterns";
    return NullPatternsVariant();
  }

  // Parse first to validate the content
  ParsedPatternsVariant result = ParsePatternsContent(uncompressed_contents);

  // Check if parsing failed (helper function returns null variant on failure)
  if (features::ShouldUseV2Patterns()) {
    if (!std::get<std::unique_ptr<V2PatternsGroup>>(result)) {
      return result;  // Already contains null variant from helper
    }
  } else {
    if (!std::get<std::unique_ptr<PatternsGroup>>(result)) {
      return result;  // Already contains null variant from helper
    }
  }

  if (!base::WriteFile(patterns_path, uncompressed_contents)) {
    VLOG(1) << "Failed to write patterns file";
    return NullPatternsVariant();
  }

  return result;
}

ParsedPatternsVariant ReadAndParsePatternsFile(base::FilePath patterns_path) {
  std::string contents;
  if (!base::ReadFileToStringWithMaxSize(patterns_path, &contents,
                                         kPatternsMaxFileSize)) {
    VLOG(1) << "Failed to read local patterns file";
    return NullPatternsVariant();
  }

  return ParsePatternsContent(contents);
}

std::unique_ptr<ServerConfig> ProcessConfigResponses(
    const std::string collector_response_body,
    const std::string quorum_response_body) {
  base::AssertLongCPUWorkAllowed();
  auto collector_parsed_json = base::JSONReader::ReadAndReturnValueWithError(
      collector_response_body, base::JSON_PARSE_RFC);
  auto quorum_parsed_json = base::JSONReader::ReadAndReturnValueWithError(
      quorum_response_body, base::JSON_PARSE_RFC);

  if (!collector_parsed_json.has_value() || !quorum_parsed_json.has_value()) {
    const auto& error = !collector_parsed_json.has_value()
                            ? collector_parsed_json.error()
                            : quorum_parsed_json.error();
    VLOG(1) << "Failed to parse server config json: " << error.ToString();
    return nullptr;
  }

  const auto* collector_root = collector_parsed_json.value().GetIfDict();
  const auto* quorum_root = quorum_parsed_json.value().GetIfDict();
  if (!collector_root || !quorum_root) {
    VLOG(1) << "Failed to parse server config: not a dict";
    return nullptr;
  }

  const auto min_version = collector_root->FindInt(kMinVersionFieldName);
  if (min_version && *min_version > kCurrentVersion) {
    VLOG(1) << "Server minimum version is higher than current version, failing";
    return nullptr;
  }

  auto config = std::make_unique<ServerConfig>();

  const auto* group_pub_keys = collector_root->FindDict(kGroupPubKeysFieldName);
  if (!group_pub_keys) {
    VLOG(1) << "Failed to retrieve groupPubKeys from server config";
    return nullptr;
  }
  const auto* pub_keys = collector_root->FindDict(kPubKeysFieldName);
  if (!pub_keys) {
    VLOG(1) << "Failed to retrieve pubKeys from server config";
    return nullptr;
  }
  const auto* source_map = collector_root->FindDict(kSourceMapFieldName);
  const auto* source_map_actions =
      source_map ? source_map->FindDict(kSourceMapActionsFieldName) : nullptr;
  if (!source_map_actions) {
    VLOG(1) << "Failed to retrieve sourceMap from server config";
    return nullptr;
  }

  const auto* location = quorum_root->FindString(kLocationFieldName);
  if (location && kAllowedReportLocations.contains(*location)) {
    config->location = std::move(*location);
  } else {
    config->location = kOmittedLocationValue;
  }

  config->group_pub_keys = ParseKeys(*group_pub_keys);
  config->pub_keys = ParseKeys(*pub_keys);
  config->source_map_actions = ParseSourceMapActionConfigs(*source_map_actions);

  return config;
}

}  // namespace

SourceMapActionConfig::SourceMapActionConfig() = default;
SourceMapActionConfig::~SourceMapActionConfig() = default;

ServerConfig::ServerConfig() = default;
ServerConfig::~ServerConfig() = default;

ServerConfigDownloadResult::ServerConfigDownloadResult(
    bool is_collector_config,
    std::optional<std::string> response_body)
    : is_collector_config(is_collector_config),
      response_body(std::move(response_body)) {}
ServerConfigDownloadResult::~ServerConfigDownloadResult() = default;
ServerConfigDownloadResult::ServerConfigDownloadResult(
    const ServerConfigDownloadResult&) = default;

ServerConfigLoader::ServerConfigLoader(
    PrefService* local_state,
    base::FilePath user_data_dir,
    network::SharedURLLoaderFactory* shared_url_loader_factory,
    base::RepeatingClosure config_callback,
    base::RepeatingClosure patterns_callback)
    : local_state_(local_state),
      background_task_runner_(
          base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()})),
      shared_url_loader_factory_(shared_url_loader_factory),
      config_callback_(config_callback),
      patterns_callback_(patterns_callback),
      config_backoff_entry_(&kBackoffPolicy),
      patterns_backoff_entry_(&kBackoffPolicy) {
  collector_config_url_ =
      GURL(GetAnonymousHPNHost() + kCollectorConfigPathWithFields);
  quorum_config_url_ = GURL(GetQuorumHost() + kQuorumConfigPath);
  patterns_url_ = GetPatternsEndpoint();

  if (features::ShouldUseV2Patterns()) {
    patterns_path_ = user_data_dir.AppendASCII(kV2PatternsFilename);
  } else {
    patterns_path_ = user_data_dir.AppendASCII(kPatternsFilename);
  }
}

ServerConfigLoader::~ServerConfigLoader() = default;

const ServerConfig& ServerConfigLoader::GetLastServerConfig() const {
  CHECK(last_loaded_server_config_);
  return *last_loaded_server_config_;
}

const PatternsGroup& ServerConfigLoader::GetLastPatterns() const {
  CHECK(last_loaded_patterns_);
  return *last_loaded_patterns_;
}

const V2PatternsGroup& ServerConfigLoader::GetLastV2Patterns() const {
  CHECK(last_loaded_v2_patterns_);
  return *last_loaded_v2_patterns_;
}

void ServerConfigLoader::SetLastServerConfigForTesting(
    std::unique_ptr<ServerConfig> server_config) {
  last_loaded_server_config_ = std::move(server_config);
}

void ServerConfigLoader::SetLastPatternsForTesting(
    std::unique_ptr<PatternsGroup> patterns) {
  last_loaded_patterns_ = std::move(patterns);
}

void ServerConfigLoader::SetLastV2PatternsForTesting(
    std::unique_ptr<V2PatternsGroup> v2_patterns) {
  last_loaded_v2_patterns_ = std::move(v2_patterns);
}

void ServerConfigLoader::LoadConfigs() {
  if (collector_config_url_loader_ || quorum_config_url_loader_) {
    // Another request is in progress
    return;
  }
  auto collector_resource_request =
      CreateResourceRequest(collector_config_url_);
  auto quorum_resource_request = CreateResourceRequest(quorum_config_url_);

  collector_config_url_loader_ = network::SimpleURLLoader::Create(
      std::move(collector_resource_request), kNetworkTrafficAnnotation);
  quorum_config_url_loader_ = network::SimpleURLLoader::Create(
      std::move(quorum_resource_request), kNetworkTrafficAnnotation);

  auto callback = base::BarrierCallback<ServerConfigDownloadResult>(
      2, base::BindOnce(&ServerConfigLoader::OnConfigResponsesDownloaded,
                        base::Unretained(this)));

  auto make_download_result = [](bool is_collector_config,
                                 std::optional<std::string> response_body) {
    return ServerConfigDownloadResult(is_collector_config,
                                      std::move(response_body));
  };

  auto collector_callback =
      base::BindOnce(make_download_result, true).Then(callback);
  auto quorum_callback =
      base::BindOnce(make_download_result, false).Then(callback);

  collector_config_url_loader_->DownloadToString(
      shared_url_loader_factory_.get(), std::move(collector_callback),
      kMaxResponseSize);
  quorum_config_url_loader_->DownloadToString(shared_url_loader_factory_.get(),
                                              std::move(quorum_callback),
                                              kMaxResponseSize);
}

void ServerConfigLoader::OnConfigResponsesDownloaded(
    std::vector<ServerConfigDownloadResult> results) {
  CHECK_EQ(results.size(), 2u);
  std::optional<std::string> collector_response_body;
  std::optional<std::string> quorum_response_body;
  for (const auto& result : results) {
    if (result.is_collector_config) {
      collector_response_body = std::move(result.response_body);
    } else {
      quorum_response_body = std::move(result.response_body);
    }
  }

  auto* collector_response_info = collector_config_url_loader_->ResponseInfo();
  auto* quorum_response_info = quorum_config_url_loader_->ResponseInfo();
  if (!collector_response_body || !quorum_response_body ||
      !collector_response_info || !quorum_response_info ||
      collector_response_info->headers->response_code() !=
          net::HttpStatusCode::HTTP_OK ||
      quorum_response_info->headers->response_code() !=
          net::HttpStatusCode::HTTP_OK) {
    VLOG(1) << "Failed to download one or more server configs";
    OnConfigResponsesProcessed(nullptr);
    return;
  }

  background_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ProcessConfigResponses,
                     std::move(*collector_response_body),
                     std::move(*quorum_response_body)),
      base::BindOnce(&ServerConfigLoader::OnConfigResponsesProcessed,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ServerConfigLoader::OnConfigResponsesProcessed(
    std::unique_ptr<ServerConfig> config) {
  bool result = config != nullptr;
  if (result) {
    last_loaded_server_config_ = std::move(config);
    config_callback_.Run();
  }

  config_backoff_entry_.InformOfRequest(result);

  collector_config_url_loader_ = nullptr;
  quorum_config_url_loader_ = nullptr;

  auto update_time = base::Time::Now();
  if (!result) {
    update_time += config_backoff_entry_.GetTimeUntilRelease();
  } else {
    update_time += base::RandTimeDelta(kMinReloadInterval, kMaxReloadInterval);

    SchedulePatternsRequest();
  }

  config_update_timer_.Start(
      FROM_HERE, update_time,
      base::BindOnce(&ServerConfigLoader::LoadConfigs, base::Unretained(this)));
}

void ServerConfigLoader::LoadStoredPatterns() {
  background_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&ReadAndParsePatternsFile, patterns_path_),
      base::BindOnce(&ServerConfigLoader::OnPatternsParsed,
                     weak_ptr_factory_.GetWeakPtr(), true));
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
  auto* response_info = patterns_url_loader_->ResponseInfo();
  if (!response_body || !response_info ||
      response_info->headers->response_code() != net::HttpStatusCode::HTTP_OK) {
    VLOG(1) << "Failed to retrieve patterns file";
    HandlePatternsStatus(false);
    return;
  }
  background_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ParseAndWritePatternsFile, patterns_path_,
                     std::move(*response_body)),
      base::BindOnce(&ServerConfigLoader::OnPatternsParsed,
                     weak_ptr_factory_.GetWeakPtr(), false));
}

void ServerConfigLoader::OnPatternsParsed(bool is_stored,
                                          ParsedPatternsVariant patterns) {
  // Move the patterns immediately based on feature flag
  if (features::ShouldUseV2Patterns()) {
    last_loaded_v2_patterns_ =
        std::move(std::get<std::unique_ptr<V2PatternsGroup>>(patterns));
  } else {
    last_loaded_patterns_ =
        std::move(std::get<std::unique_ptr<PatternsGroup>>(patterns));
  }

  // Check if patterns were successfully loaded
  if (!last_loaded_patterns_ && !last_loaded_v2_patterns_) {
    if (is_stored) {
      local_state_->ClearPref(kPatternsRetrievalTime);
      SchedulePatternsRequest();
    } else {
      HandlePatternsStatus(false);
    }
    return;
  }

  if (!is_stored) {
    local_state_->SetTime(kPatternsRetrievalTime, base::Time::Now());
    HandlePatternsStatus(true);
  }

  patterns_callback_.Run();
}

}  // namespace web_discovery
