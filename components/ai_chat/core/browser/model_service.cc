// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/model_service.h"

#include <algorithm>
#include <ios>
#include <iterator>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

#include "base/base64.h"
#include "base/check.h"
#include "base/containers/checked_iterators.h"
#include "base/containers/contains.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/metrics/field_trial_params.h"
#include "base/no_destructor.h"
#include "base/numerics/safe_math.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/uuid.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer_claude.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer_conversation_api.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer_llama.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer_oai.h"
#include "brave/components/ai_chat/core/browser/model_validator.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "cc/task/core/task_utils.h"
#include "components/os_crypt/sync/os_crypt.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace ai_chat {
class AIChatCredentialManager;

inline constexpr char kDefaultModelKey[] = "brave.ai_chat.default_model_key";
inline constexpr char kCustomModelsList[] = "brave.ai_chat.custom_models";
namespace {
inline constexpr char kCustomModelItemLabelKey[] = "label";
inline constexpr char kCustomModelItemModelKey[] = "model_request_name";
inline constexpr char kCustomModelContextSizeKey[] = "context_size";
inline constexpr char kCustomModelSystemPromptKey[] = "model_system_prompt";
inline constexpr char kCustomModelItemEndpointUrlKey[] = "endpoint_url";
inline constexpr char kCustomModelItemApiKey[] = "api_key";
inline constexpr char kCustomModelItemKey[] = "key";

// When adding new models, especially for display, make sure to add the UI
// strings to ai_chat_ui_strings.grdp and ai_chat/core/constants.cc.
// This also applies for modifying keys, since some of the strings are based
// on the model key. Also be sure to migrate prefs if changing or removing
// keys.

// Llama2 Token Allocation:
// - Llama2 has a context limit: tokens + max_new_tokens <= 4096
//
// Breakdown:
// - Reserved for max_new_tokens: 400 tokens
// - Reserved for prompt: 300 tokens
// - Reserved for page content: 4096 - (400 + 300) = 3396 tokens
// - Long conversation warning threshold: 3396 * 0.80 = 2716 tokens

// Claude Token Allocation:
// - Claude has total token limit 100k tokens (75k words)
//
// Breakdown:
// - Reserverd for page content: 100k / 2 = 50k tokens
// - Long conversation warning threshold: 100k * 0.80 = 80k tokens

const std::vector<mojom::ModelPtr>& GetLeoModels() {
  // TODO(petemill): When removing kFreemiumAvailable flag, and not having any
  // BASIC and PREMIUM-only models, remove all the `switchToBasicModel`-related
  // functions.
  static const base::NoDestructor<std::vector<mojom::ModelPtr>> kModels([]() {
    static const auto kFreemiumAccess =
        features::kFreemiumAvailable.Get()
            ? mojom::ModelAccess::BASIC_AND_PREMIUM
            : mojom::ModelAccess::PREMIUM;
    const bool conversation_api = features::kConversationAPIEnabled.Get();

    std::vector<mojom::ModelPtr> models;
    {
      auto options = mojom::LeoModelOptions::New();
      options->display_maker = "Mistral AI";
      options->name = "mixtral-8x7b-instruct";
      options->category = mojom::ModelCategory::CHAT;
      options->access = kFreemiumAccess;
      options->engine_type =
          conversation_api ? mojom::ModelEngineType::BRAVE_CONVERSATION_API
                           : mojom::ModelEngineType::LLAMA_REMOTE;
      options->max_associated_content_length = 8000;
      options->long_conversation_warning_character_limit = 9700;

      auto model = mojom::Model::New();
      model->key = "chat-leo-expanded";
      model->display_name = "Mixtral";
      model->options =
          mojom::ModelOptions::NewLeoModelOptions(std::move(options));

      models.push_back(std::move(model));
    }

    {
      auto options = mojom::LeoModelOptions::New();
      options->display_maker = "Anthropic";
      options->name = "claude-3-haiku";
      options->category = mojom::ModelCategory::CHAT;
      options->access = kFreemiumAccess;
      options->engine_type =
          conversation_api ? mojom::ModelEngineType::BRAVE_CONVERSATION_API
                           : mojom::ModelEngineType::CLAUDE_REMOTE;
      options->max_associated_content_length = 180000;
      options->long_conversation_warning_character_limit = 320000;

      auto model = mojom::Model::New();
      model->key = "chat-claude-haiku";
      model->display_name = "Claude 3 Haiku";
      model->options =
          mojom::ModelOptions::NewLeoModelOptions(std::move(options));

      models.push_back(std::move(model));
    }

    {
      auto options = mojom::LeoModelOptions::New();
      options->display_maker = "Anthropic";
      options->name = "claude-3-sonnet";
      options->category = mojom::ModelCategory::CHAT;
      options->access = mojom::ModelAccess::PREMIUM;
      options->engine_type =
          conversation_api ? mojom::ModelEngineType::BRAVE_CONVERSATION_API
                           : mojom::ModelEngineType::CLAUDE_REMOTE;
      options->max_associated_content_length = 180000;
      options->long_conversation_warning_character_limit = 320000;

      auto model = mojom::Model::New();
      model->key = "chat-claude-sonnet";
      model->display_name = "Claude 3.5 Sonnet";
      model->options =
          mojom::ModelOptions::NewLeoModelOptions(std::move(options));

      models.push_back(std::move(model));
    }

    {
      auto options = mojom::LeoModelOptions::New();
      options->display_maker = "Meta";
      options->name = "llama-3-8b-instruct";
      options->category = mojom::ModelCategory::CHAT;
      options->access = features::kFreemiumAvailable.Get()
                            ? mojom::ModelAccess::BASIC_AND_PREMIUM
                            : mojom::ModelAccess::BASIC;
      options->engine_type =
          conversation_api ? mojom::ModelEngineType::BRAVE_CONVERSATION_API
                           : mojom::ModelEngineType::LLAMA_REMOTE;
      options->max_associated_content_length = 8000;
      options->long_conversation_warning_character_limit = 9700;

      auto model = mojom::Model::New();
      model->key = "chat-basic";
      model->display_name = "Llama 3.1 8B";
      model->options =
          mojom::ModelOptions::NewLeoModelOptions(std::move(options));

      models.push_back(std::move(model));
    }

    return models;
  }());

  return *kModels;
}

// TODO(nullhook): Handle encryption/decryption failures
std::string EncryptAPIKey(const std::string& api_key) {
  if (api_key.empty()) {
    return std::string();
  }

  std::string encrypted_api_key;
  if (!OSCrypt::EncryptString(api_key, &encrypted_api_key)) {
    VLOG(1) << "Encrypt api key failure";
    return std::string();
  }

  return base::Base64Encode(encrypted_api_key);
}

std::string DecryptAPIKey(const std::string& encoded_api_key) {
  if (encoded_api_key.empty()) {
    return std::string();
  }

  std::string encrypted_api_key;
  if (!base::Base64Decode(encoded_api_key, &encrypted_api_key)) {
    VLOG(1) << "base64 decode api key failure";
    return std::string();
  }

  std::string api_key;
  if (!OSCrypt::DecryptString(encrypted_api_key, &api_key)) {
    VLOG(1) << "Decrypt api key failure";
    return std::string();
  }

  return api_key;
}

base::Value::Dict GetModelDict(mojom::ModelPtr model) {
  base::Value::Dict model_dict = base::Value::Dict();

  mojom::CustomModelOptions options =
      *model->options->get_custom_model_options();

  model_dict.Set(kCustomModelItemKey, model->key);
  model_dict.Set(kCustomModelItemLabelKey, model->display_name);
  model_dict.Set(kCustomModelItemModelKey, options.model_request_name);
  model_dict.Set(kCustomModelItemEndpointUrlKey, options.endpoint.spec());
  model_dict.Set(kCustomModelItemApiKey, EncryptAPIKey(options.api_key));
  model_dict.Set(kCustomModelContextSizeKey,
                 static_cast<int32_t>(options.context_size));

  // Check if the model has a user-specified system prompt
  if (options.model_system_prompt.has_value() &&
      !options.model_system_prompt->empty()) {
    model_dict.Set(kCustomModelSystemPromptKey,
                   options.model_system_prompt.value());
  }

  return model_dict;
}

}  // namespace

ModelService::ModelService(PrefService* prefs_service)
    : pref_service_(prefs_service) {
  InitModels();
  // Perform migrations which depend on finding out about user's premium status
  const std::string& default_model_user_pref = GetDefaultModelKey();
  if (default_model_user_pref == "chat-claude-instant") {
    // 2024-05 Migration for old "claude instant" model
    // The migration is performed here instead of
    // ai_chat::prefs::MigrateProfilePrefs because the migration requires
    // knowing about premium status.
    // First set to an equivalent model that is available to all users. When
    // we are told about premium status, we can switch to the premium
    // equivalent.
    SetDefaultModelKey("chat-claude-haiku");
    is_migrating_claude_instant_ = true;
  }
}

ModelService::~ModelService() = default;

// static
void ModelService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kCustomModelsList, {});
  registry->RegisterStringPref(kDefaultModelKey,
                               features::kAIModelsDefaultKey.Get());
}

// static
void ModelService::MigrateProfilePrefs(PrefService* profile_prefs) {
  if (ai_chat::features::IsAIChatEnabled()) {
    profile_prefs->ClearPref(prefs::kObseleteBraveChatAutoGenerateQuestions);

    // migrate model key from "chat-default" to "chat-basic"
    static const std::string kDefaultModelBasicFrom = "chat-default";
    static const std::string kDefaultModelBasicTo = "chat-basic";
    if (auto* default_model_value =
            profile_prefs->GetUserPrefValue(kDefaultModelKey)) {
      if (base::EqualsCaseInsensitiveASCII(default_model_value->GetString(),
                                           kDefaultModelBasicFrom)) {
        profile_prefs->SetString(kDefaultModelKey, kDefaultModelBasicTo);
      }
    }
  }
}

// Custom models do not have fixed properties pertaining to the number of
// characters they can process before a potential-coherence-loss warning is
// shown. Leo Models have hard-coded values, but custom models' properties are
// based on their context size, which may or may not have been provided by the
// user. For this reason, we set the long_conversation_warning_character_limit
// and max_associated_content_length after the model has been loaded and
// validated.
void ModelService::SetAssociatedContentLengthMetrics(mojom::Model& model) {
  if (!model.options->is_custom_model_options()) {
    // Only set metrics for custom models
    return;
  }

  if (!ModelValidator::HasValidContextSize(
          *model.options->get_custom_model_options())) {
    model.options->get_custom_model_options()->context_size =
        kDefaultCustomModelContextSize;
  }

  uint32_t max_associated_content_length =
      ModelService::CalcuateMaxAssociatedContentLengthForModel(model);

  model.options->get_custom_model_options()->max_associated_content_length =
      max_associated_content_length;

  base::CheckedNumeric<uint32> warn_at = base::CheckMul<size_t>(
      max_associated_content_length, kMaxContentLengthThreshold);

  if (warn_at.IsValid()) {
    model.options->get_custom_model_options()
        ->long_conversation_warning_character_limit = warn_at.ValueOrDie();
  }
}

// static
size_t ModelService::CalcuateMaxAssociatedContentLengthForModel(
    const mojom::Model& model) {
  if (model.options->is_leo_model_options()) {
    return model.options->get_leo_model_options()
        ->max_associated_content_length;
  }

  const auto context_size =
      model.options->get_custom_model_options()->context_size;

  constexpr uint32_t reserved_tokens =
      kReservedTokensForMaxNewTokens + kReservedTokensForPrompt;

  // CheckedNumerics for safe math
  base::CheckedNumeric<size_t> safeContextSize(context_size);
  base::CheckedNumeric<size_t> safeReservedTokens(reserved_tokens);
  base::CheckedNumeric<size_t> safeCharsPerToken(kDefaultCharsPerToken);

  return ((safeContextSize - safeReservedTokens) * safeCharsPerToken)
      .ValueOrDie();
}

// static
const mojom::Model* ModelService::GetModelForTesting(std::string_view key) {
  const std::vector<mojom::ModelPtr>& all_models = GetLeoModels();

  auto match_iter = std::find_if(
      all_models.cbegin(), all_models.cend(),
      [&key](const mojom::ModelPtr& model) { return model->key == key; });
  if (match_iter != all_models.cend()) {
    return &*match_iter->get();
  }

  return nullptr;
}

void ModelService::OnPremiumStatus(mojom::PremiumStatus status) {
  if (is_migrating_claude_instant_) {
    is_migrating_claude_instant_ = false;
    if (status != mojom::PremiumStatus::Inactive) {
      SetDefaultModelKey("chat-claude-sonnet");
    }
  } else if (IsPremiumStatus(status)) {
    // If user hasn't changed default model and we configure that premium
    // default model is different to non-premium default model, then change to
    // premium default model.
    const base::Value* user_value =
        pref_service_->GetUserPrefValue(kDefaultModelKey);
    if (!user_value &&
        features::kAIModelsDefaultKey.Get() !=
            features::kAIModelsPremiumDefaultKey.Get() &&
        GetDefaultModelKey() != features::kAIModelsPremiumDefaultKey.Get()) {
      // We don't call SetDefaultModelKey as we don't want to actually set
      // the pref value for the user, we only want to change the default so
      // that the user benefits from future changes to the default.
      pref_service_->SetDefaultPrefValue(
          kDefaultModelKey,
          base::Value(features::kAIModelsPremiumDefaultKey.Get()));
      for (auto& obs : observers_) {
        obs.OnDefaultModelChanged(features::kAIModelsDefaultKey.Get(),
                                  features::kAIModelsPremiumDefaultKey.Get());
      }
    }
  }
}

void ModelService::InitModels() {
  // Get leo and custom models
  const std::vector<mojom::ModelPtr>& leo_models = GetLeoModels();
  std::vector<mojom::ModelPtr> custom_models = GetCustomModelsFromPrefs();

  // Reserve space in the combined models vector
  models_.clear();
  models_.reserve(leo_models.size() + custom_models.size());

  // Ensure we return only in intended display order
  std::transform(leo_models.cbegin(), leo_models.cend(),
                 std::back_inserter(models_),
                 [](const mojom::ModelPtr& model) { return model.Clone(); });

  std::transform(custom_models.cbegin(), custom_models.cend(),
                 std::back_inserter(models_),
                 [](const mojom::ModelPtr& model) { return model.Clone(); });

  for (auto& obs : observers_) {
    obs.OnModelListUpdated();
  }
}

const std::vector<mojom::ModelPtr>& ModelService::GetModels() {
  return models_;
}

const mojom::Model* ModelService::GetModel(std::string_view key) {
  const std::vector<mojom::ModelPtr>& all_models = GetModels();

  auto match_iter = std::find_if(
      all_models.cbegin(), all_models.cend(),
      [&key](const mojom::ModelPtr& model) { return model->key == key; });
  if (match_iter != all_models.cend()) {
    return &*match_iter->get();
  }

  return nullptr;
}

void ModelService::AddCustomModel(mojom::ModelPtr model) {
  CHECK(model->key.empty()) << "Model key should be empty for new models.";

  model->key = base::StrCat(
      {"custom:",
       base::Uuid::GenerateRandomV4().AsLowercaseString().substr(0, 8)});

  // Validate the model
  ModelValidationResult result = ModelValidator::ValidateCustomModelOptions(
      *model->options->get_custom_model_options());
  if (result != ModelValidationResult::kSuccess) {
    if (result == ModelValidationResult::kInvalidContextSize) {
      VLOG(2) << "Invalid context size for model: " << model->key;
      model->options->get_custom_model_options()->context_size =
          kDefaultCustomModelContextSize;
    }
  }

  base::Value::List custom_models_pref =
      pref_service_->GetList(kCustomModelsList).Clone();
  base::Value::Dict model_dict = GetModelDict(std::move(model));
  custom_models_pref.Append(std::move(model_dict));
  pref_service_->SetList(kCustomModelsList, std::move(custom_models_pref));

  InitModels();
}

void ModelService::SaveCustomModel(uint32_t index, mojom::ModelPtr model) {
  // Validate the model
  ModelValidationResult result = ModelValidator::ValidateCustomModelOptions(
      *model->options->get_custom_model_options());
  if (result != ModelValidationResult::kSuccess) {
    if (result == ModelValidationResult::kInvalidContextSize) {
      VLOG(2) << "Invalid context size for model: " << model->key;
      model->options->get_custom_model_options()->context_size =
          kDefaultCustomModelContextSize;
    }
  }

  // Set metrics for AI Chat content length warnings
  SetAssociatedContentLengthMetrics(*model);

  base::Value::List custom_models_pref =
      pref_service_->GetList(kCustomModelsList).Clone();

  if (index >= custom_models_pref.size() || index < 0) {
    return;
  }

  auto model_iter = custom_models_pref.begin() + index;

  const std::string& existing_key =
      *model_iter->GetDict().FindString(kCustomModelItemKey);

  // Make sure the key is not changed when modifying the model
  // because Dict::Merge is destructive.
  CHECK(existing_key == model->key)
      << "Model key mismatch. Existing key: " << existing_key
      << ", sent model key: " << model->key << ".";

  base::Value::Dict model_dict = GetModelDict(std::move(model));
  model_iter->GetDict().Merge(std::move(model_dict));

  pref_service_->SetList(kCustomModelsList, std::move(custom_models_pref));

  InitModels();
}

void ModelService::DeleteCustomModel(uint32_t index) {
  base::Value::List custom_models_pref =
      pref_service_->GetList(kCustomModelsList).Clone();

  if (index >= custom_models_pref.size() || index < 0) {
    return;
  }

  auto model = custom_models_pref.begin() + index;
  std::string removed_key = *model->GetDict().FindString(kCustomModelItemKey);

  auto current_default_key = GetDefaultModelKey();

  // If the removed model is the default model, clear the default model key.
  if (current_default_key == removed_key) {
    pref_service_->ClearPref(kDefaultModelKey);
    DVLOG(1) << "Default model key " << removed_key
             << " was removed. Cleared default model key.";
    for (auto& obs : observers_) {
      obs.OnDefaultModelChanged(removed_key, GetDefaultModelKey());
    }
  }

  custom_models_pref.erase(model);
  pref_service_->SetList(kCustomModelsList, std::move(custom_models_pref));

  InitModels();

  for (auto& obs : observers_) {
    obs.OnModelRemoved(removed_key);
  }
}

void ModelService::SetDefaultModelKey(const std::string& new_key) {
  const auto& models = GetModels();

  bool does_model_exist = base::Contains(
      models, new_key, [](const mojom::ModelPtr& model) { return model->key; });

  if (!does_model_exist) {
    DVLOG(1) << "Default model key " << new_key
             << " does not exist in the model list.";
    return;
  }

  // Don't continue migrating if user choses another default in the meantime
  is_migrating_claude_instant_ = false;

  const std::string previous_default_key = GetDefaultModelKey();

  if (previous_default_key == new_key) {
    // Nothing to do
    return;
  }

  pref_service_->SetString(kDefaultModelKey, new_key);

  for (auto& obs : observers_) {
    obs.OnDefaultModelChanged(previous_default_key, new_key);
  }
}

void ModelService::SetDefaultModelKeyWithoutValidationForTesting(
    const std::string& model_key) {
  pref_service_->SetString(kDefaultModelKey, model_key);
}

const std::string& ModelService::GetDefaultModelKey() {
  return pref_service_->GetString(kDefaultModelKey);
}

std::vector<mojom::ModelPtr> ModelService::GetCustomModelsFromPrefs() {
  std::vector<mojom::ModelPtr> models;

  const base::Value::List& custom_models_pref =
      pref_service_->GetList(kCustomModelsList);

  for (const base::Value& item : custom_models_pref) {
    const base::Value::Dict& model_pref = item.GetDict();
    auto custom_model_opts = mojom::CustomModelOptions::New();
    custom_model_opts->model_request_name =
        *model_pref.FindString(kCustomModelItemModelKey);
    custom_model_opts->endpoint =
        GURL(*model_pref.FindString(kCustomModelItemEndpointUrlKey));
    custom_model_opts->context_size =
        model_pref.FindInt(kCustomModelContextSizeKey)
            .value_or(kDefaultCustomModelContextSize);
    custom_model_opts->api_key =
        DecryptAPIKey(*model_pref.FindString(kCustomModelItemApiKey));

    // Populate system prompt (if it exists)
    if (const std::string* model_system_prompt =
            model_pref.FindString(kCustomModelSystemPromptKey)) {
      custom_model_opts->model_system_prompt = *model_system_prompt;
    }

    auto model = mojom::Model::New();
    model->key = *model_pref.FindString(kCustomModelItemKey);
    model->display_name = *model_pref.FindString(kCustomModelItemLabelKey);
    model->options = mojom::ModelOptions::NewCustomModelOptions(
        std::move(custom_model_opts));

    // Validate the model
    ModelValidationResult result = ModelValidator::ValidateCustomModelOptions(
        *model->options->get_custom_model_options());
    if (result != ModelValidationResult::kSuccess) {
      if (result == ModelValidationResult::kInvalidContextSize) {
        VLOG(2) << "Invalid context size for model: " << model->key;
        model->options->get_custom_model_options()->context_size =
            kDefaultCustomModelContextSize;
      }
    }

    // Set metrics for AI Chat content length warnings
    SetAssociatedContentLengthMetrics(*model);

    models.push_back(std::move(model));
  }

  return models;
}

void ModelService::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void ModelService::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

std::unique_ptr<EngineConsumer> ModelService::GetEngineForModel(
    std::string model_key,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    AIChatCredentialManager* credential_manager) {
  const mojom::Model* model = GetModel(model_key);
  std::unique_ptr<EngineConsumer> engine;
  // Only LeoModels are passed to the following engines.
  if (model->options->is_leo_model_options()) {
    auto& leo_model_opts = model->options->get_leo_model_options();

    // Engine enum on model to decide which one
    if (leo_model_opts->engine_type ==
        mojom::ModelEngineType::BRAVE_CONVERSATION_API) {
      DVLOG(1) << "Started AI engine: conversation api";
      engine = std::make_unique<EngineConsumerConversationAPI>(
          *leo_model_opts, url_loader_factory, credential_manager);
    } else if (leo_model_opts->engine_type ==
               mojom::ModelEngineType::LLAMA_REMOTE) {
      DVLOG(1) << "Started AI engine: llama";
      engine = std::make_unique<EngineConsumerLlamaRemote>(
          *leo_model_opts, url_loader_factory, credential_manager);
    } else {
      DVLOG(1) << "Started AI engine: claude";
      engine = std::make_unique<EngineConsumerClaudeRemote>(
          *leo_model_opts, url_loader_factory, credential_manager);
    }
  } else if (model->options->is_custom_model_options()) {
    auto& custom_model_opts = model->options->get_custom_model_options();
    DVLOG(1) << "Started AI engine: custom";
    engine = std::make_unique<EngineConsumerOAIRemote>(*custom_model_opts,
                                                       url_loader_factory);
  }

  return engine;
}

}  // namespace ai_chat
