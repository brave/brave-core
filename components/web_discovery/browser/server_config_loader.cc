/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/server_config_loader.h"

#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/location.h"
#include "base/rand_util.h"
#include "base/values.h"
#include "brave/components/web_discovery/browser/util.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace web_discovery {

namespace {

constexpr net::BackoffEntry::Policy kBackoffPolicy = {
    .num_errors_to_ignore = 0,
    .initial_delay_ms = 10 * 1000,
    .multiply_factor = 2.0,
    .jitter_factor = 0.1,
    .maximum_backoff_ms = 10 * 60 * 1000,
    .entry_lifetime_ms = -1,
    .always_use_initial_delay = false};

constexpr base::TimeDelta kMinReloadInterval = base::Hours(1);
constexpr base::TimeDelta kMaxReloadInterval = base::Hours(4);

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

}  // namespace

ServerConfig::ServerConfig() = default;

ServerConfig::~ServerConfig() = default;

ServerConfigLoader::ServerConfigLoader(
    network::SharedURLLoaderFactory* shared_url_loader_factory,
    ConfigCallback config_callback)
    : shared_url_loader_factory_(shared_url_loader_factory),
      config_callback_(config_callback),
      backoff_entry_(&kBackoffPolicy) {
  config_url_ =
      GURL(base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                         GetCollectorHost(), kConfigPathWithFields}));
}

ServerConfigLoader::~ServerConfigLoader() = default;

void ServerConfigLoader::Load() {
  // TODO(djandries): create a backoff url loader if one doenst exist
  // but consider how join retries will work before you do. we might want to
  // generate gsk beforehand... in which case it might make sense to add a
  // repeating callback for generating state + network requests
  if (url_loader_) {
    // Another request is in progress
    return;
  }
  auto resource_request = CreateResourceRequest(config_url_);

  url_loader_ = network::SimpleURLLoader::Create(std::move(resource_request),
                                                 kNetworkTrafficAnnotation);

  url_loader_->DownloadToString(
      shared_url_loader_factory_.get(),
      base::BindOnce(&ServerConfigLoader::OnConfigResponse,
                     base::Unretained(this)),
      kMaxResponseSize);
}

void ServerConfigLoader::OnConfigResponse(
    std::optional<std::string> response_body) {
  base::Time update_time = base::Time::Now();
  bool result = !ProcessConfigResponse(response_body);

  backoff_entry_.InformOfRequest(response_body.has_value());

  if (!result) {
    update_time += backoff_entry_.GetTimeUntilRelease();
  } else {
    update_time += base::RandTimeDelta(kMinReloadInterval, kMaxReloadInterval);
  }

  update_timer_.Start(
      FROM_HERE, update_time,
      base::BindOnce(&ServerConfigLoader::Load, base::Unretained(this)));
}

bool ServerConfigLoader::ProcessConfigResponse(
    const std::optional<std::string>& response_body) {
  auto* response_info = url_loader_->ResponseInfo();
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

}  // namespace web_discovery
