// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/remote_models_fetcher.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
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
constexpr char kVisionSupportField[] = "vision_support";
constexpr char kSupportsToolsField[] = "supports_tools";
constexpr char kAudioSupportField[] = "audio_support";
constexpr char kVideoSupportField[] = "video_support";
constexpr char kSupportedCapabilitiesField[] = "supported_capabilities";
constexpr char kIsSuggestedModelField[] = "is_suggested_model";
constexpr char kIsNearModelField[] = "is_near_model";
constexpr char kOptionsField[] = "options";

constexpr char kTypeField[] = "type";
constexpr char kNameField[] = "name";
constexpr char kDisplayMakerField[] = "display_maker";
constexpr char kDescriptionField[] = "description";
constexpr char kCategoryField[] = "category";
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
          "This feature can be disabled by setting the remote_models_endpoint "
          "feature parameter to an empty string."
      })");

mojom::ModelCategory ParseCategory(const std::string& category_str) {
  if (category_str == "chat") {
    return mojom::ModelCategory::CHAT;
  } else if (category_str == "summary") {
    return mojom::ModelCategory::SUMMARY;
  }
  DVLOG(1) << "Unknown model category: " << category_str;
  return mojom::ModelCategory::CHAT;
}

std::optional<mojom::ModelAccess> ParseAccess(const std::string& access_str) {
  if (access_str == "basic") {
    return mojom::ModelAccess::BASIC;
  } else if (access_str == "premium") {
    return mojom::ModelAccess::PREMIUM;
  } else if (access_str == "basic_and_premium") {
    return mojom::ModelAccess::BASIC_AND_PREMIUM;
  }
  DVLOG(1) << "Unknown model access: " << access_str;
  return std::nullopt;
}

std::optional<mojom::ConversationCapability> ParseCapability(
    const std::string& capability_str) {
  if (capability_str == "chat") {
    return mojom::ConversationCapability::CHAT;
  } else if (capability_str == "content_agent") {
    return mojom::ConversationCapability::CONTENT_AGENT;
  } else if (capability_str == "deep_research") {
    return mojom::ConversationCapability::DEEP_RESEARCH;
  }
  DVLOG(1) << "Unknown conversation capability: " << capability_str;
  return std::nullopt;
}

mojom::ModelPtr ParseModel(const base::DictValue& model_dict) {
  const std::string* key = model_dict.FindString(kKeyField);
  if (!key || key->empty()) {
    DVLOG(1) << "Model missing or has empty 'key' field";
    return nullptr;
  }

  const std::string* display_name = model_dict.FindString(kDisplayNameField);
  if (!display_name || display_name->empty()) {
    DVLOG(1) << "Model missing or has empty 'display_name' field: " << *key;
    return nullptr;
  }

  const base::DictValue* options_dict = model_dict.FindDict(kOptionsField);
  if (!options_dict) {
    DVLOG(1) << "Model missing 'options' field: " << *key;
    return nullptr;
  }

  const std::string* type = options_dict->FindString(kTypeField);
  if (!type || *type != "leo") {
    DVLOG(1) << "Model options must have type='leo': " << *key;
    return nullptr;
  }

  const std::string* name = options_dict->FindString(kNameField);
  if (!name || name->empty()) {
    DVLOG(1) << "Model options missing or has empty 'name' field: " << *key;
    return nullptr;
  }

  std::optional<int> max_content_length =
      options_dict->FindInt(kMaxAssociatedContentLengthField);
  if (max_content_length.has_value() && *max_content_length <= 0) {
    DVLOG(1) << "Model has invalid max_associated_content_length: " << *key;
    return nullptr;
  }

  std::optional<int> warning_limit =
      options_dict->FindInt(kLongConversationWarningCharacterLimitField);
  if (warning_limit.has_value() && *warning_limit <= 0) {
    DVLOG(1) << "Model has invalid long_conversation_warning_character_limit: "
             << *key;
    return nullptr;
  }

  const std::string* access = options_dict->FindString(kAccessField);
  std::optional<mojom::ModelAccess> parsed_access =
      access ? ParseAccess(*access) : std::nullopt;
  if (!parsed_access.has_value()) {
    return nullptr;
  }

  auto model = mojom::Model::New();
  model->key = *key;
  model->display_name = *display_name;
  model->vision_support =
      model_dict.FindBool(kVisionSupportField).value_or(false);
  model->supports_tools =
      model_dict.FindBool(kSupportsToolsField).value_or(false);
  model->audio_support =
      model_dict.FindBool(kAudioSupportField).value_or(false);
  model->video_support =
      model_dict.FindBool(kVideoSupportField).value_or(false);
  model->is_suggested_model =
      model_dict.FindBool(kIsSuggestedModelField).value_or(false);
  model->is_near_model = model_dict.FindBool(kIsNearModelField).value_or(false);

  if (const base::ListValue* capabilities_list =
          model_dict.FindList(kSupportedCapabilitiesField)) {
    for (const auto& capability_value : *capabilities_list) {
      if (!capability_value.is_string()) {
        continue;
      }
      auto capability = ParseCapability(capability_value.GetString());
      if (capability.has_value()) {
        model->supported_capabilities.push_back(*capability);
      }
    }
  }

  auto leo_opts = mojom::LeoModelOptions::New();
  leo_opts->name = *name;

  const std::string* display_maker =
      options_dict->FindString(kDisplayMakerField);
  if (display_maker) {
    leo_opts->display_maker = *display_maker;
  }

  const std::string* description = options_dict->FindString(kDescriptionField);
  leo_opts->description = description ? *description : "";

  const std::string* category = options_dict->FindString(kCategoryField);
  leo_opts->category =
      category ? ParseCategory(*category) : mojom::ModelCategory::CHAT;

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
  GURL endpoint_url(url);

  if (!endpoint_url.is_valid() || !endpoint_url.SchemeIs(url::kHttpsScheme)) {
    std::move(callback).Run({});
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

std::vector<mojom::ModelPtr> RemoteModelsFetcher::ParseModelsFromJSON(
    const base::Value& json) {
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

void RemoteModelsFetcher::OnFetchComplete(
    FetchModelsCallback callback,
    api_request_helper::APIRequestResult result) {
  if (!result.Is2XXResponseCode()) {
    std::move(callback).Run({});
    return;
  }

  std::move(callback).Run(ParseModelsFromJSON(result.value_body()));
}

}  // namespace ai_chat
