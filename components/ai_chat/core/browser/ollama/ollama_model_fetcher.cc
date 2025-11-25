/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ollama/ollama_model_fetcher.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/task/sequenced_task_runner.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/ollama/ollama_service.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

constexpr uint32_t kDefaultContextSize = 8192;

}  // namespace

OllamaModelFetcher::OllamaModelFetcher(ModelService& model_service,
                                       PrefService* prefs,
                                       OllamaService* ollama_service)
    : model_service_(model_service),
      prefs_(prefs),
      ollama_service_(ollama_service) {
  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(
      prefs::kBraveAIChatOllamaFetchEnabled,
      base::BindRepeating(&OllamaModelFetcher::OnOllamaFetchEnabledChanged,
                          weak_ptr_factory_.GetWeakPtr()));

  // Trigger initial Ollama fetch if enabled
  if (prefs_->GetBoolean(prefs::kBraveAIChatOllamaFetchEnabled)) {
    DVLOG(1) << "Ollama fetch is enabled on startup - triggering initial fetch";
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&OllamaModelFetcher::FetchModels,
                                  weak_ptr_factory_.GetWeakPtr()));
  }
}

OllamaModelFetcher::~OllamaModelFetcher() = default;

void OllamaModelFetcher::OnOllamaFetchEnabledChanged() {
  bool ollama_fetch_enabled =
      prefs_->GetBoolean(prefs::kBraveAIChatOllamaFetchEnabled);

  if (ollama_fetch_enabled) {
    FetchModels();
  }
}

void OllamaModelFetcher::FetchModels() {
  ollama_service_->FetchModels(base::BindOnce(
      &OllamaModelFetcher::OnModelsFetched, weak_ptr_factory_.GetWeakPtr()));
}

void OllamaModelFetcher::OnModelsFetched(
    std::optional<std::vector<std::string>> models) {
  if (!models) {
    return;
  }

  const auto& existing_ollama_models = model_service_->GetCustomModels();
  std::set<std::string> existing_ollama_model_names;

  // Find existing Ollama models
  for (const auto& existing_model : existing_ollama_models) {
    if (existing_model->options &&
        existing_model->options->is_custom_model_options() &&
        existing_model->options->get_custom_model_options() &&
        existing_model->options->get_custom_model_options()
            ->endpoint.is_valid() &&
        existing_model->options->get_custom_model_options()->endpoint.spec() ==
            mojom::kOllamaEndpoint) {
      const std::string& model_name =
          existing_model->options->get_custom_model_options()
              ->model_request_name;
      existing_ollama_model_names.insert(model_name);
    }
  }

  // Build set of current Ollama models from the response
  std::set<std::string> current_ollama_models;
  for (const auto& model_name : *models) {
    current_ollama_models.insert(model_name);
  }

  // Remove Ollama models that are no longer available
  model_service_->MaybeDeleteCustomModels(base::BindRepeating(
      [](const std::set<std::string>& current_models,
         const base::Value::Dict& model_dict) {
        const std::string* endpoint_str =
            model_dict.FindString(kCustomModelItemEndpointUrlKey);
        const std::string* model_name =
            model_dict.FindString(kCustomModelItemModelKey);

        // Only remove Ollama models that are not in the current list
        return endpoint_str && model_name &&
               GURL(*endpoint_str) == GURL(mojom::kOllamaEndpoint) &&
               !current_models.contains(*model_name);
      },
      current_ollama_models));

  // Clear pending models map before processing new models
  pending_models_.clear();

  // Fetch detailed information for each new model
  for (const auto& model_name : *models) {
    if (existing_ollama_model_names.contains(model_name)) {
      continue;
    }

    // Store basic info and fetch detailed information
    PendingModelInfo pending_info;
    pending_info.model_name = model_name;
    pending_info.display_name = model_name;
    pending_models_[model_name] = std::move(pending_info);

    FetchModelDetails(model_name);
  }
}

void OllamaModelFetcher::FetchModelDetails(const std::string& model_name) {
  ollama_service_->ShowModel(
      model_name, base::BindOnce(&OllamaModelFetcher::OnModelDetailsFetched,
                                 weak_ptr_factory_.GetWeakPtr(), model_name));
}

void OllamaModelFetcher::OnModelDetailsFetched(
    const std::string& model_name,
    std::optional<OllamaService::ModelDetails> details) {
  auto it = pending_models_.find(model_name);
  if (it == pending_models_.end()) {
    return;
  }

  uint32_t context_size = kDefaultContextSize;
  bool vision_support = false;

  if (details) {
    if (details->context_length > 0) {
      context_size = details->context_length;
    }
    vision_support = details->has_vision;
  }

  // Create custom model for Ollama with detailed information
  auto custom_model = mojom::Model::New();
  custom_model->key = "";  // Empty for new models
  custom_model->display_name = it->second.display_name;
  custom_model->vision_support = vision_support;
  custom_model->supports_tools = false;
  custom_model->is_suggested_model = false;

  auto custom_options = mojom::CustomModelOptions::New();
  custom_options->model_request_name = model_name;
  custom_options->endpoint = GURL(mojom::kOllamaEndpoint);
  custom_options->api_key = "";  // Ollama doesn't require authentication
  custom_options->context_size = context_size;

  custom_model->options =
      mojom::ModelOptions::NewCustomModelOptions(std::move(custom_options));

  model_service_->AddCustomModel(std::move(custom_model));

  // Remove from pending models
  pending_models_.erase(it);
}

}  // namespace ai_chat
