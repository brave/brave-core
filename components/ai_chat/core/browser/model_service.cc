// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/model_service.h"

#include <algorithm>
#include <utility>

#include "base/base64.h"
#include "base/containers/contains.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/uuid.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/os_crypt/sync/os_crypt.h"
#include "components/prefs/pref_service.h"

namespace ai_chat {

inline constexpr char kDefaultModelKey[] = "brave.ai_chat.default_model_key";
inline constexpr char kCustomModelsList[] = "brave.ai_chat.custom_models";
namespace {
inline constexpr char kCustomModelItemLabelKey[] = "label";
inline constexpr char kCustomModelItemModelKey[] = "model_request_name";
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
      options->max_page_content_length = 8000;
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
      options->max_page_content_length = 180000;
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
      options->max_page_content_length = 180000;
      options->long_conversation_warning_character_limit = 320000;

      auto model = mojom::Model::New();
      model->key = "chat-claude-sonnet";
      model->display_name = "Claude 3 Sonnet";
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
      options->max_page_content_length = 8000;
      options->long_conversation_warning_character_limit = 9700;

      auto model = mojom::Model::New();
      model->key = "chat-basic";
      model->display_name = "Llama 3 8b";
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
  model_dict.Set(kCustomModelItemLabelKey, model->display_name);
  model_dict.Set(
      kCustomModelItemModelKey,
      model->options->get_custom_model_options()->model_request_name);
  model_dict.Set(kCustomModelItemEndpointUrlKey,
                 model->options->get_custom_model_options()->endpoint.spec());
  model_dict.Set(
      kCustomModelItemApiKey,
      EncryptAPIKey(model->options->get_custom_model_options()->api_key));
  model_dict.Set(kCustomModelItemKey, model->key);
  return model_dict;
}

}  // namespace

ModelService::ModelService(PrefService* prefs_service)
    : pref_service_(prefs_service) {
  InitModels();

  pref_change_registrar_.Init(pref_service_);
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

  base::Value::List custom_models_pref =
      pref_service_->GetList(kCustomModelsList).Clone();
  base::Value::Dict model_dict = GetModelDict(std::move(model));
  custom_models_pref.Append(std::move(model_dict));
  pref_service_->SetList(kCustomModelsList, std::move(custom_models_pref));

  InitModels();
}

void ModelService::SaveCustomModel(uint32_t index, mojom::ModelPtr model) {
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

  custom_models_pref.erase(model);
  pref_service_->SetList(kCustomModelsList, std::move(custom_models_pref));

  auto current_default_key = pref_service_->GetString(kDefaultModelKey);

  // If the removed model is the default model, clear the default model key.
  if (current_default_key == removed_key) {
    pref_service_->ClearPref(kDefaultModelKey);
    DVLOG(1) << "Default model key " << removed_key
             << " was removed. Cleared default model key.";
  }

  InitModels();
  for (auto& obs : observers_) {
    obs.OnModelRemoved(removed_key);
    obs.OnDefaultModelChanged(features::kAIModelsDefaultKey.Get());
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

  pref_service_->SetString(kDefaultModelKey, new_key);

  for (auto& obs : observers_) {
    obs.OnDefaultModelChanged(new_key);
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
    custom_model_opts->api_key =
        DecryptAPIKey(*model_pref.FindString(kCustomModelItemApiKey));

    auto model = mojom::Model::New();
    model->key = *model_pref.FindString(kCustomModelItemKey);
    model->display_name = *model_pref.FindString(kCustomModelItemLabelKey);
    model->options = mojom::ModelOptions::NewCustomModelOptions(
        std::move(custom_model_opts));

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

}  // namespace ai_chat
