// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/oblivious_http_config_manager.h"

#include <utility>

#include "base/base64.h"
#include "base/functional/bind.h"
#include "base/json/values_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "net/http/http_request_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

constexpr char kKeyConfigPathFormat[] = "v1/models/%s/ohttp_config";
constexpr base::TimeDelta kKeyConfigExpiresAfter = base::Days(3);

constexpr char kKeyConfigKey[] = "key_config";
constexpr char kEndpointUrlKey[] = "endpoint_url";
constexpr char kUpstreamModelNameKey[] = "upstream_model_name";
constexpr char kPrefExpiresAtField[] = "expires_at";

bool IsExpired(const base::DictValue& entry) {
  const base::Value* expires_val = entry.Find(kPrefExpiresAtField);
  std::optional<base::Time> expires_at =
      expires_val ? base::ValueToTime(expires_val) : std::nullopt;
  return !expires_at || base::Time::Now() >= *expires_at;
}

net::NetworkTrafficAnnotationTag GetKeyConfigTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("ai_chat_ohttp_key_config", R"cpp(
    semantics{
      sender : "AI Chat (OHTTP Key Config)" description :
          "Fetches the HPKE public key configuration and inner endpoint URL "
          "for a specific model from the Brave AI Chat server at "
          "/v1/models/{model_name}/ohttp_config. The response contains a "
          "base64-encoded key config blob and the endpoint URL used for the "
          "OHTTP inner request."
      trigger :
          "Fetched on demand when a private inference request is about to be "
          "sent for a model with no valid cached key config." data :
              "No user data is sent. This is a plain GET request for a public "
              "key configuration document." destination : WEBSITE
    } policy{
      cookies_allowed : NO policy_exception_justification : "Not implemented."
    }
  )cpp");
}

}  // namespace

ObliviousHttpConfigManager::ObliviousHttpConfigManager(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* profile_prefs)
    : url_loader_factory_(std::move(url_loader_factory)),
      profile_prefs_(profile_prefs) {}

ObliviousHttpConfigManager::~ObliviousHttpConfigManager() = default;

// static
void ObliviousHttpConfigManager::DeleteExpiredKeyConfigs(
    PrefService* profile_prefs) {
  const base::DictValue& all =
      profile_prefs->GetDict(prefs::kAIChatObliviousHttpKeyConfigs);
  std::vector<std::string> expired_keys;
  for (auto [model_name, entry_val] : all) {
    if (!entry_val.is_dict() || IsExpired(entry_val.GetDict())) {
      expired_keys.push_back(model_name);
    }
  }
  if (expired_keys.empty()) {
    return;
  }
  ScopedDictPrefUpdate update(profile_prefs,
                              prefs::kAIChatObliviousHttpKeyConfigs);
  for (const auto& key : expired_keys) {
    update->Remove(key);
  }
}

std::optional<ObliviousHttpConfigManager::KeyConfigResult>
ObliviousHttpConfigManager::ParseKeyConfig(const base::Value& value,
                                           const std::string& model_name,
                                           bool save) {
  if (!value.is_dict()) {
    return std::nullopt;
  }
  const base::DictValue& dict = value.GetDict();

  const std::string* key_config_b64 = dict.FindString(kKeyConfigKey);
  const std::string* endpoint_url_str = dict.FindString(kEndpointUrlKey);
  const std::string* upstream_model_name =
      dict.FindString(kUpstreamModelNameKey);
  if (!key_config_b64 || key_config_b64->empty() || !endpoint_url_str ||
      endpoint_url_str->empty() || !upstream_model_name ||
      upstream_model_name->empty()) {
    return std::nullopt;
  }

  auto decoded = base::Base64Decode(*key_config_b64);
  if (!decoded) {
    return std::nullopt;
  }

  GURL endpoint_url(*endpoint_url_str);
  if (!endpoint_url.is_valid() || !endpoint_url.SchemeIs("https")) {
    return std::nullopt;
  }

  if (save) {
    ScopedDictPrefUpdate update(profile_prefs_,
                                prefs::kAIChatObliviousHttpKeyConfigs);
    base::DictValue entry;
    entry.Set(kKeyConfigKey, *key_config_b64);
    entry.Set(kEndpointUrlKey, endpoint_url.spec());
    entry.Set(kUpstreamModelNameKey, *upstream_model_name);
    entry.Set(kPrefExpiresAtField,
              base::TimeToValue(base::Time::Now() + kKeyConfigExpiresAfter));
    update->Set(model_name, std::move(entry));
  }

  return KeyConfigResult{std::string(decoded->begin(), decoded->end()),
                         std::move(endpoint_url), *upstream_model_name};
}

std::optional<ObliviousHttpConfigManager::KeyConfigResult>
ObliviousHttpConfigManager::GetCachedKeyConfig(const std::string& model_name) {
  const base::DictValue* entry =
      profile_prefs_->GetDict(prefs::kAIChatObliviousHttpKeyConfigs)
          .FindDict(model_name);
  if (!entry || IsExpired(*entry)) {
    return std::nullopt;
  }
  return ParseKeyConfig(base::Value(entry->Clone()), model_name,
                        /*save=*/false);
}

void ObliviousHttpConfigManager::RequestKeyConfig(const std::string& model_name,
                                                  KeyConfigCallback callback) {
  if (auto cached = GetCachedKeyConfig(model_name)) {
    std::move(callback).Run(std::move(cached));
    return;
  }
  FetchKeyConfig(model_name, std::move(callback));
}

void ObliviousHttpConfigManager::CancelAll() {
  if (api_request_helper_) {
    api_request_helper_->CancelAll();
  }
}

void ObliviousHttpConfigManager::ClearKeyConfig(const std::string& model_name) {
  ScopedDictPrefUpdate update(profile_prefs_,
                              prefs::kAIChatObliviousHttpKeyConfigs);
  update->Remove(model_name);
}

void ObliviousHttpConfigManager::FetchKeyConfig(const std::string& model_name,
                                                KeyConfigCallback callback) {
  if (!api_request_helper_) {
    api_request_helper_ =
        std::make_unique<api_request_helper::APIRequestHelper>(
            GetKeyConfigTrafficAnnotationTag(), url_loader_factory_);
  }
  api_request_helper_->Request(
      net::HttpRequestHeaders::kGetMethod,
      GetEndpointUrl(/*premium=*/false,
                     absl::StrFormat(kKeyConfigPathFormat, model_name)),
      /*payload=*/"", /*payload_content_type=*/"",
      base::BindOnce(&ObliviousHttpConfigManager::OnKeyConfigFetched,
                     weak_factory_.GetWeakPtr(), model_name,
                     std::move(callback)),
      GetBraveHeaders(/*credential=*/std::nullopt));
}

void ObliviousHttpConfigManager::OnKeyConfigFetched(
    std::string model_name,
    KeyConfigCallback callback,
    api_request_helper::APIRequestResult result) {
  if (!result.Is2XXResponseCode()) {
    std::move(callback).Run(std::nullopt);
    return;
  }
  std::move(callback).Run(
      ParseKeyConfig(result.value_body(), model_name, /*save=*/true));
}

}  // namespace ai_chat
