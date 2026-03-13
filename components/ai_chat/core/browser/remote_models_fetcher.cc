// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/remote_models_fetcher.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "components/prefs/pref_service.h"
#include "net/base/url_util.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

constexpr size_t kMaxResponseSize = 5 * 1024 * 1024;  // 5MB
constexpr char kModelsKey[] = "models";
constexpr char kLastUpdatedKey[] = "last_updated";
constexpr char kEndpointUrlKey[] = "endpoint_url";

// JSON field names
constexpr char kKeyField[] = "key";
constexpr char kDisplayNameField[] = "display_name";
constexpr char kVisionSupportField[] = "vision_support";
constexpr char kSupportsToolsField[] = "supports_tools";
constexpr char kAudioSupportField[] = "audio_support";
constexpr char kVideoSupportField[] = "video_support";
constexpr char kIsSuggestedModelField[] = "is_suggested_model";
constexpr char kIsNearModelField[] = "is_near_model";
constexpr char kOptionsField[] = "options";

// LeoModelOptions fields
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
          "requiring browser updates. Sends the browser's UI language "
          "to allow localized model names."
        trigger:
          "Triggered on browser startup or when the model cache expires "
          "(default: 1 hour TTL)."
        data:
          "Browser UI language code as a query parameter (e.g., 'en', 'es', "
          "'fr'). This is user-identifying metadata that can serve as a "
          "fingerprinting signal."
        destination: BRAVE_OWNED_SERVICE
        internal {
          contacts {
            email: "support@brave.com"
          }
        }
        user_data {
          type: OTHER
        }
        last_reviewed: "2025-02-18"
      }
      policy {
        cookies_allowed: NO
        setting:
          "This feature can be disabled by setting the remote_models_endpoint "
          "feature parameter to an empty string."
        policy_exception_justification:
          "Not implemented yet. If this feature is enabled by default, "
          "a policy will be added to disable it."
      })");

mojom::ModelCategory ParseCategory(const std::string& category_str) {
  if (category_str == "chat") {
    return mojom::ModelCategory::CHAT;
  }
  VLOG(1) << "Unknown model category: " << category_str;
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
  VLOG(1) << "Unknown model access: " << access_str;
  return std::nullopt;
}

std::string GetTwoLetterLanguageCode() {
  // Get the application locale (e.g., "en-US", "es-ES", "fr-FR")
  // Pass empty string to get the system/configured locale
  std::string locale = l10n_util::GetApplicationLocale("");

  // Extract first two characters (language code)
  if (locale.length() >= 2) {
    return locale.substr(0, 2);
  }

  // Fallback to English
  return "en";
}

}  // namespace

RemoteModelsFetcher::RemoteModelsFetcher(
    PrefService* pref_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : pref_service_(pref_service),
      api_request_helper_(
          std::make_unique<api_request_helper::APIRequestHelper>(
              kTrafficAnnotation,
              url_loader_factory)) {}

RemoteModelsFetcher::~RemoteModelsFetcher() = default;

void RemoteModelsFetcher::FetchModels(const std::string& url,
                                      FetchModelsCallback callback) {
  GURL endpoint_url(url);

  // Validate URL (HTTPS only, except localhost for testing)
  if (!endpoint_url.is_valid() || endpoint_url.is_empty()) {
    VLOG(1) << "Invalid remote models endpoint URL: " << url;
    std::move(callback).Run({});
    return;
  }

  if (!endpoint_url.SchemeIs(url::kHttpsScheme) &&
      !(endpoint_url.SchemeIs(url::kHttpScheme) &&
        endpoint_url.host() == "localhost")) {
    VLOG(1) << "Remote models endpoint must use HTTPS: " << url;
    std::move(callback).Run({});
    return;
  }

  // Add language query parameter
  std::string lang_code = GetTwoLetterLanguageCode();
  GURL url_with_lang =
      net::AppendQueryParameter(endpoint_url, "lang", lang_code);

  VLOG(1) << "Fetching remote models from: " << url_with_lang.spec()
          << " (language: " << lang_code << ")";

  // Create a wrapper callback that adapts APIRequestResult to our callback
  auto result_callback = base::BindOnce(
      [](base::WeakPtr<RemoteModelsFetcher> fetcher, std::string original_url,
         FetchModelsCallback callback,
         api_request_helper::APIRequestResult result) {
        VLOG(1) << "Anonymous callback";
        if (!fetcher) {
          return;
        }
        fetcher->OnFetchComplete(original_url, std::move(callback),
                                 std::move(result));
      },
      weak_ptr_factory_.GetWeakPtr(), url, std::move(callback));

  api_request_helper::APIRequestOptions options;
  options.max_body_size = kMaxResponseSize;
  options.timeout = base::Seconds(30);

  api_request_helper_->Request(
      "GET", url_with_lang, "", "", std::move(result_callback),
      base::flat_map<std::string, std::string>(), options);
}

bool RemoteModelsFetcher::HasValidCache() const {
  if (cached_models_.empty()) {
    return false;
  }

  base::TimeDelta ttl =
      base::Minutes(features::kRemoteModelsCacheTTLMinutes.Get());
  return (base::Time::Now() - cache_timestamp_) < ttl;
}

bool RemoteModelsFetcher::IsStale() const {
  if (cached_models_.empty()) {
    return false;
  }

  base::TimeDelta ttl =
      base::Minutes(features::kRemoteModelsCacheTTLMinutes.Get());
  return (base::Time::Now() - cache_timestamp_) >= ttl;
}

std::vector<mojom::ModelPtr> RemoteModelsFetcher::GetCachedModels() const {
  std::vector<mojom::ModelPtr> result;
  result.reserve(cached_models_.size());
  for (const auto& model : cached_models_) {
    result.push_back(model->Clone());
  }
  return result;
}

std::vector<mojom::ModelPtr> RemoteModelsFetcher::LoadFromPrefs() {
  const base::DictValue& cache_dict =
      pref_service_->GetDict(prefs::kRemoteModelsCache);

  if (cache_dict.empty()) {
    VLOG(1) << "No remote models cache found in prefs";
    return {};
  }

  const base::ListValue* models_list = cache_dict.FindList(kModelsKey);
  if (!models_list) {
    VLOG(1) << "Invalid remote models cache in prefs (no models list)";
    return {};
  }

  std::optional<double> last_updated = cache_dict.FindDouble(kLastUpdatedKey);
  const std::string* endpoint_url = cache_dict.FindString(kEndpointUrlKey);

  if (!last_updated.has_value() || !endpoint_url) {
    VLOG(1) << "Invalid remote models cache in prefs (missing metadata)";
    return {};
  }

  // Parse models from the list
  base::Value models_value(models_list->Clone());
  std::vector<mojom::ModelPtr> models = ParseModelsFromJSON(models_value);

  if (models.empty()) {
    VLOG(1) << "Failed to parse models from prefs cache";
    return {};
  }

  // Update in-memory cache
  cached_models_.clear();
  cached_models_.reserve(models.size());
  for (const auto& model : models) {
    cached_models_.push_back(model->Clone());
  }
  cache_timestamp_ = base::Time::FromSecondsSinceUnixEpoch(*last_updated);
  cached_endpoint_url_ = *endpoint_url;

  VLOG(1) << "Loaded " << models.size() << " remote models from prefs cache";
  return models;
}

void RemoteModelsFetcher::ClearCache() {
  cached_models_.clear();
  cache_timestamp_ = base::Time();
  cached_endpoint_url_.clear();
  VLOG(1) << "Cleared in-memory remote models cache";
}

void RemoteModelsFetcher::SaveToPrefs(
    const std::vector<mojom::ModelPtr>& models,
    const std::string& endpoint_url) {
  base::DictValue cache_dict;

  // Serialize models
  base::ListValue models_list;
  for (const auto& model : models) {
    base::DictValue model_dict;
    model_dict.Set(kKeyField, model->key);
    model_dict.Set(kDisplayNameField, model->display_name);
    model_dict.Set(kVisionSupportField, model->vision_support);
    model_dict.Set(kSupportsToolsField, model->supports_tools);
    model_dict.Set(kAudioSupportField, model->audio_support);
    model_dict.Set(kVideoSupportField, model->video_support);
    model_dict.Set(kIsSuggestedModelField, model->is_suggested_model);
    model_dict.Set(kIsNearModelField, model->is_near_model);

    // Serialize LeoModelOptions
    if (model->options->is_leo_model_options()) {
      const auto& leo_opts = model->options->get_leo_model_options();
      base::DictValue options_dict;
      options_dict.Set(kTypeField, "leo");
      options_dict.Set(kNameField, leo_opts->name);
      if (!leo_opts->display_maker.empty()) {
        options_dict.Set(kDisplayMakerField, leo_opts->display_maker);
      }
      options_dict.Set(kDescriptionField, leo_opts->description);
      options_dict.Set(kCategoryField,
                       leo_opts->category == mojom::ModelCategory::CHAT
                           ? "chat"
                           : "unknown");
      std::string access_str;
      switch (leo_opts->access) {
        case mojom::ModelAccess::BASIC:
          access_str = "basic";
          break;
        case mojom::ModelAccess::PREMIUM:
          access_str = "premium";
          break;
        case mojom::ModelAccess::BASIC_AND_PREMIUM:
          access_str = "basic_and_premium";
          break;
      }
      options_dict.Set(kAccessField, access_str);
      options_dict.Set(
          kMaxAssociatedContentLengthField,
          static_cast<int>(leo_opts->max_associated_content_length));
      options_dict.Set(
          kLongConversationWarningCharacterLimitField,
          static_cast<int>(
              leo_opts->long_conversation_warning_character_limit));

      model_dict.Set(kOptionsField, std::move(options_dict));
    }

    models_list.Append(std::move(model_dict));
  }

  cache_dict.Set(kModelsKey, std::move(models_list));
  cache_dict.Set(kLastUpdatedKey, base::Time::Now().InSecondsFSinceUnixEpoch());
  cache_dict.Set(kEndpointUrlKey, endpoint_url);

  pref_service_->SetDict(prefs::kRemoteModelsCache, std::move(cache_dict));
  VLOG(1) << "Saved " << models.size() << " remote models to prefs cache";
}

std::vector<mojom::ModelPtr> RemoteModelsFetcher::ParseModelsFromJSON(
    const base::Value& json) {
  std::vector<mojom::ModelPtr> models;

  const base::ListValue* models_list = nullptr;

  // Handle both {"models": [...]} and direct array [...]
  if (json.is_dict()) {
    models_list = json.GetDict().FindList(kModelsKey);
  } else if (json.is_list()) {
    models_list = &json.GetList();
  }

  if (!models_list) {
    VLOG(1) << "Invalid JSON structure: expected 'models' array";
    return {};
  }

  for (const auto& model_value : *models_list) {
    if (!model_value.is_dict()) {
      VLOG(1) << "Skipping non-dict model entry";
      continue;
    }

    const base::DictValue& model_dict = model_value.GetDict();

    if (!ValidateModel(model_dict)) {
      VLOG(1) << "Skipping invalid model entry";
      continue;
    }

    // Parse model
    auto model = mojom::Model::New();
    model->key = *model_dict.FindString(kKeyField);
    model->display_name = *model_dict.FindString(kDisplayNameField);
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
    model->is_near_model =
        model_dict.FindBool(kIsNearModelField).value_or(false);

    // Parse options
    const base::DictValue* options_dict = model_dict.FindDict(kOptionsField);
    if (!options_dict) {
      VLOG(1) << "Skipping model without options: " << model->key;
      continue;
    }

    const std::string* type = options_dict->FindString(kTypeField);
    if (!type || *type != "leo") {
      VLOG(1) << "Skipping model with non-leo type: " << model->key;
      continue;
    }

    auto leo_opts = mojom::LeoModelOptions::New();
    leo_opts->name = *options_dict->FindString(kNameField);

    const std::string* display_maker =
        options_dict->FindString(kDisplayMakerField);
    if (display_maker) {
      leo_opts->display_maker = *display_maker;
    }

    const std::string* description =
        options_dict->FindString(kDescriptionField);
    leo_opts->description = description ? *description : "";
    const std::string* category = options_dict->FindString(kCategoryField);
    leo_opts->category =
        category ? ParseCategory(*category) : mojom::ModelCategory::CHAT;

    const std::string* access = options_dict->FindString(kAccessField);
    std::optional<mojom::ModelAccess> parsed_access =
        access ? ParseAccess(*access) : std::nullopt;
    if (!parsed_access.has_value()) {
      VLOG(1) << "Skipping model with unrecognized access level: "
              << model->key;
      continue;
    }
    leo_opts->access = parsed_access.value();

    std::optional<int> max_content_length =
        options_dict->FindInt(kMaxAssociatedContentLengthField);
    std::optional<int> warning_limit =
        options_dict->FindInt(kLongConversationWarningCharacterLimitField);

    if (!max_content_length.has_value() || max_content_length.value() <= 0) {
      max_content_length =
          (leo_opts->access == mojom::ModelAccess::PREMIUM) ? 90000 : 32000;
      VLOG(1) << "Using default max_associated_content_length for "
              << model->key << ": " << max_content_length.value();
    }

    if (!warning_limit.has_value() || warning_limit.value() <= 0) {
      warning_limit =
          (leo_opts->access == mojom::ModelAccess::PREMIUM) ? 160000 : 51200;
      VLOG(1) << "Using default long_conversation_warning_character_limit for "
              << model->key << ": " << warning_limit.value();
    }

    leo_opts->max_associated_content_length = max_content_length.value();
    leo_opts->long_conversation_warning_character_limit = warning_limit.value();

    model->options =
        mojom::ModelOptions::NewLeoModelOptions(std::move(leo_opts));

    models.push_back(std::move(model));
  }

  VLOG(1) << "Parsed " << models.size() << " models from JSON";
  return models;
}

bool RemoteModelsFetcher::ValidateModel(
    const base::DictValue& model_dict) const {
  // Required fields
  const std::string* key = model_dict.FindString(kKeyField);
  const std::string* display_name = model_dict.FindString(kDisplayNameField);
  const base::DictValue* options = model_dict.FindDict(kOptionsField);

  if (!key || key->empty()) {
    VLOG(1) << "Model missing or has empty 'key' field";
    return false;
  }

  // Reject keys starting with "custom:" (reserved for custom models)
  if (base::StartsWith(*key, "custom:", base::CompareCase::INSENSITIVE_ASCII)) {
    VLOG(1) << "Model key cannot start with 'custom:': " << *key;
    return false;
  }

  if (!display_name || display_name->empty()) {
    VLOG(1) << "Model missing or has empty 'display_name' field: " << *key;
    return false;
  }

  if (!options) {
    VLOG(1) << "Model missing 'options' field: " << *key;
    return false;
  }

  // Validate options
  const std::string* type = options->FindString(kTypeField);
  if (!type || *type != "leo") {
    VLOG(1) << "Model options must have type='leo': " << *key;
    return false;
  }

  const std::string* name = options->FindString(kNameField);
  if (!name || name->empty()) {
    VLOG(1) << "Model options missing or has empty 'name' field: " << *key;
    return false;
  }

  std::optional<int> max_content =
      options->FindInt(kMaxAssociatedContentLengthField);
  if (max_content.has_value() && *max_content <= 0) {
    VLOG(1) << "Model has invalid max_associated_content_length: " << *key;
    return false;
  }

  std::optional<int> warning_limit =
      options->FindInt(kLongConversationWarningCharacterLimitField);
  if (warning_limit.has_value() && *warning_limit <= 0) {
    VLOG(1) << "Model has invalid long_conversation_warning_character_limit: "
            << *key;
    return false;
  }

  return true;
}

void RemoteModelsFetcher::OnFetchComplete(
    const std::string& original_url,
    FetchModelsCallback callback,
    api_request_helper::APIRequestResult result) {
  if (!result.Is2XXResponseCode()) {
    VLOG(1) << "Remote models fetch failed with HTTP status: "
            << result.response_code();
    std::move(callback).Run({});
    return;
  }

  const base::Value& response_body = result.value_body();
  if (!response_body.is_dict() && !response_body.is_list()) {
    VLOG(1) << "Remote models response is not a dict or list";
    std::move(callback).Run({});
    return;
  }

  std::vector<mojom::ModelPtr> models = ParseModelsFromJSON(response_body);

  if (models.empty()) {
    VLOG(1) << "No valid models found in remote response";
    std::move(callback).Run({});
    return;
  }

  // Update in-memory cache
  cached_models_.clear();
  cached_models_.reserve(models.size());
  for (const auto& m : models) {
    cached_models_.push_back(m->Clone());
  }
  cache_timestamp_ = base::Time::Now();
  cached_endpoint_url_ = original_url;

  // Save to persistent storage
  SaveToPrefs(models, cached_endpoint_url_);

  VLOG(1) << "Successfully fetched and cached " << models.size()
          << " remote models";

  std::move(callback).Run(std::move(models));
}

}  // namespace ai_chat
