// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/remote_models_fetcher.h"

#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

constexpr size_t kMaxResponseSize = 5 * 1024 * 1024;  // 5MB
constexpr char kModelsKey[] = "models";

constexpr char kKeyField[] = "key";
constexpr char kDisplayNameField[] = "display_name";
constexpr char kCapabilitiesField[] = "capabilities";
// User-facing selector tags (Fast, Vision, etc.), distinct from conversation
// capabilities above.
constexpr char kModelCapabilitiesField[] = "model_capabilities";
constexpr char kIsSuggestedModelField[] = "is_suggested_model";
constexpr char kIsNearModelField[] = "is_near_model";
constexpr char kOptionsField[] = "options";

constexpr char kTypeField[] = "type";
constexpr char kNameField[] = "name";
constexpr char kDisplayMakerField[] = "display_maker";
constexpr char kDescriptionField[] = "description";
constexpr char kAccessField[] = "access";
constexpr char kMaxAssociatedContentLengthField[] =
    "max_associated_content_length";
constexpr char kLongConversationWarningCharacterLimitField[] =
    "long_conversation_warning_character_limit";

constexpr net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("ai_chat_remote_models_fetcher", R"(
      semantics {
        sender: "AI Chat Remote Models Fetcher"
        description:
          "Fetches the list of available AI chat models from a remote "
          "endpoint. This allows dynamic model configuration without "
          "requiring browser updates."
        trigger:
          "Triggered when the user is opted in to AI Chat and the AI Chat "
          "panel is opened, if the model cache is empty or expired."
        data:
          "The Accept-Language header is included automatically by the "
          "network stack, indicating the user's preferred languages. This "
          "can serve as a fingerprinting signal."
        destination: BRAVE_OWNED_SERVICE
        internal {
          contacts {
            email: "support@brave.com"
          }
        }
        user_data {
          type: OTHER
        }
        last_reviewed: "2026-04-17"
      }
      policy {
        cookies_allowed: NO
        setting:
          "This feature can be disabled via the AIChatRemoteModelsConfig "
          "feature flag."
      })");

std::optional<mojom::ModelAccess> ParseAccess(const std::string& access_str) {
  if (access_str == "basic") {
    return mojom::ModelAccess::BASIC;
  }
  if (access_str == "premium") {
    return mojom::ModelAccess::PREMIUM;
  }
  if (access_str == "basic_and_premium") {
    return mojom::ModelAccess::BASIC_AND_PREMIUM;
  }
  DVLOG(1) << "Unknown model access: " << access_str;
  return std::nullopt;
}

std::optional<mojom::ConversationCapability> ParseCapability(
    const std::string& capability_str) {
  if (capability_str == "chat") {
    return mojom::ConversationCapability::CHAT;
  }
  if (capability_str == "content_agent") {
    return mojom::ConversationCapability::CONTENT_AGENT;
  }
  if (capability_str == "deep_research") {
    return mojom::ConversationCapability::DEEP_RESEARCH;
  }
  if (capability_str == "files") {
    return mojom::ConversationCapability::FILES;
  }
  if (capability_str == "summary") {
    return mojom::ConversationCapability::SUMMARY;
  }
  DVLOG(1) << "Unknown conversation capability: " << capability_str;
  return std::nullopt;
}

std::optional<mojom::ModelCapability> ParseModelCapability(
    const std::string& capability_str) {
  if (capability_str == "fast") {
    return mojom::ModelCapability::FAST;
  }
  if (capability_str == "thinking") {
    return mojom::ModelCapability::THINKING;
  }
  if (capability_str == "search") {
    return mojom::ModelCapability::SEARCH;
  }
  if (capability_str == "vision") {
    return mojom::ModelCapability::VISION;
  }
  if (capability_str == "tools") {
    return mojom::ModelCapability::TOOLS;
  }
  if (capability_str == "audio") {
    return mojom::ModelCapability::AUDIO;
  }
  if (capability_str == "video") {
    return mojom::ModelCapability::VIDEO;
  }
  DVLOG(1) << "Unknown model capability: " << capability_str;
  return std::nullopt;
}

std::vector<mojom::ModelCapability> ParseModelCapabilities(
    const base::DictValue& model_dict) {
  std::vector<mojom::ModelCapability> capabilities;
  const base::ListValue* capabilities_list =
      model_dict.FindList(kModelCapabilitiesField);
  if (!capabilities_list) {
    return capabilities;
  }
  for (const auto& capability_value : *capabilities_list) {
    if (!capability_value.is_string()) {
      continue;
    }
    if (auto capability =
            ParseModelCapability(capability_value.GetString())) {
      capabilities.push_back(*capability);
    }
  }
  return capabilities;
}

struct ParsedCapabilities {
  std::vector<mojom::ConversationCapability> capabilities;
  mojom::ModelCategory category;
};

// Parses the model's capability list and derives its category from the first
// CHAT or SUMMARY capability in list order. Returns std::nullopt if the list
// is missing or declares neither category capability, in which case the model
// is rejected.
std::optional<ParsedCapabilities> ParseCapabilities(
    const base::DictValue& model_dict) {
  const base::ListValue* capabilities_list =
      model_dict.FindList(kCapabilitiesField);
  if (!capabilities_list) {
    return std::nullopt;
  }

  std::vector<mojom::ConversationCapability> capabilities;
  for (const auto& capability_value : *capabilities_list) {
    if (!capability_value.is_string()) {
      continue;
    }
    if (auto capability = ParseCapability(capability_value.GetString())) {
      capabilities.push_back(*capability);
    }
  }

  for (auto capability : capabilities) {
    if (capability == mojom::ConversationCapability::CHAT) {
      return ParsedCapabilities{std::move(capabilities),
                                mojom::ModelCategory::CHAT};
    }
    if (capability == mojom::ConversationCapability::SUMMARY) {
      return ParsedCapabilities{std::move(capabilities),
                                mojom::ModelCategory::SUMMARY};
    }
  }
  return std::nullopt;
}

mojom::ModelPtr ParseModel(const base::DictValue& model_dict) {
  const std::string* key = model_dict.FindString(kKeyField);
  if (!key || key->empty()) {
    return nullptr;
  }

  const std::string* display_name = model_dict.FindString(kDisplayNameField);
  if (!display_name || display_name->empty()) {
    return nullptr;
  }

  const base::DictValue* options_dict = model_dict.FindDict(kOptionsField);
  if (!options_dict) {
    return nullptr;
  }

  const std::string* type = options_dict->FindString(kTypeField);
  if (!type || *type != "leo") {
    return nullptr;
  }

  const std::string* name = options_dict->FindString(kNameField);
  if (!name || name->empty()) {
    return nullptr;
  }

  std::optional<int> max_content_length =
      options_dict->FindInt(kMaxAssociatedContentLengthField);
  if (max_content_length.has_value() && *max_content_length <= 0) {
    return nullptr;
  }

  std::optional<int> warning_limit =
      options_dict->FindInt(kLongConversationWarningCharacterLimitField);
  if (warning_limit.has_value() && *warning_limit <= 0) {
    return nullptr;
  }

  const std::string* access = options_dict->FindString(kAccessField);
  std::optional<mojom::ModelAccess> parsed_access =
      access ? ParseAccess(*access) : std::nullopt;
  if (!parsed_access.has_value()) {
    return nullptr;
  }

  auto parsed_capabilities = ParseCapabilities(model_dict);
  if (!parsed_capabilities) {
    return nullptr;
  }

  auto model = mojom::Model::New();
  model->key = *key;
  model->display_name = *display_name;
  model->is_suggested_model =
      model_dict.FindBool(kIsSuggestedModelField).value_or(false);
  model->is_near_model = model_dict.FindBool(kIsNearModelField).value_or(false);
  model->supported_capabilities = std::move(parsed_capabilities->capabilities);
  model->capabilities = ParseModelCapabilities(model_dict);

  auto leo_opts = mojom::LeoModelOptions::New();
  leo_opts->name = *name;

  const std::string* display_maker =
      options_dict->FindString(kDisplayMakerField);
  if (display_maker) {
    leo_opts->display_maker = *display_maker;
  }

  const std::string* description = options_dict->FindString(kDescriptionField);
  leo_opts->description = description ? *description : "";

  leo_opts->category = parsed_capabilities->category;

  leo_opts->access = *parsed_access;

  if (!max_content_length.has_value()) {
    max_content_length =
        (leo_opts->access == mojom::ModelAccess::PREMIUM) ? 90000 : 32000;
  }
  if (!warning_limit.has_value()) {
    warning_limit =
        (leo_opts->access == mojom::ModelAccess::PREMIUM) ? 160000 : 51200;
  }

  leo_opts->max_associated_content_length =
      base::saturated_cast<uint32_t>(*max_content_length);
  leo_opts->long_conversation_warning_character_limit =
      base::saturated_cast<uint32_t>(*warning_limit);

  model->options = mojom::ModelOptions::NewLeoModelOptions(std::move(leo_opts));

  return model;
}

std::vector<mojom::ModelPtr> ParseModelsFromJSON(const base::Value& json) {
  const base::ListValue* models_list = nullptr;

  if (json.is_dict()) {
    models_list = json.GetDict().FindList(kModelsKey);
  } else if (json.is_list()) {
    models_list = &json.GetList();
  }

  if (!models_list) {
    return {};
  }

  std::vector<mojom::ModelPtr> models;
  for (const auto& model_value : *models_list) {
    if (!model_value.is_dict()) {
      continue;
    }
    auto model = ParseModel(model_value.GetDict());
    if (model) {
      models.push_back(std::move(model));
    }
  }
  return models;
}

}  // namespace

RemoteModelsFetcher::RemoteModelsFetcher(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(
          std::make_unique<api_request_helper::APIRequestHelper>(
              kTrafficAnnotation,
              url_loader_factory)) {}

RemoteModelsFetcher::~RemoteModelsFetcher() = default;

void RemoteModelsFetcher::FetchModels(const std::string& url,
                                      FetchModelsCallback callback) {
  const GURL endpoint_url(url);

  if (!endpoint_url.is_valid() || !endpoint_url.SchemeIs(url::kHttpsScheme)) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), std::vector<mojom::ModelPtr>{}));
    return;
  }

  auto result_callback =
      base::BindOnce(&RemoteModelsFetcher::OnFetchComplete,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  api_request_helper::APIRequestOptions options;
  options.max_body_size = kMaxResponseSize;
  options.timeout = base::Seconds(30);

  api_request_helper_->Request(
      "GET", endpoint_url, "", "", std::move(result_callback),
      base::flat_map<std::string, std::string>(), options);
}

void RemoteModelsFetcher::OnFetchComplete(
    FetchModelsCallback callback,
    api_request_helper::APIRequestResult result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!result.Is2XXResponseCode()) {
    std::move(callback).Run({});
    return;
  }

  std::move(callback).Run(ParseModelsFromJSON(result.value_body()));
}

}  // namespace ai_chat
