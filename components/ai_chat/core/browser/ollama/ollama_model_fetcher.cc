/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ollama/ollama_model_fetcher.h"

#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/ollama/ollama_client.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "third_party/abseil-cpp/absl/strings/ascii.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

constexpr uint32_t kDefaultContextSize = 8192;

}  // namespace

OllamaModelFetcher::OllamaModelFetcher(
    ModelService& model_service,
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : model_service_(model_service),
      prefs_(prefs),
      ollama_client_(std::make_unique<OllamaClient>(url_loader_factory)) {
  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(
      prefs::kBraveAIChatOllamaFetchEnabled,
      base::BindRepeating(&OllamaModelFetcher::OnOllamaFetchEnabledChanged,
                          weak_ptr_factory_.GetWeakPtr()));
}

OllamaModelFetcher::~OllamaModelFetcher() = default;

void OllamaModelFetcher::OnOllamaFetchEnabledChanged() {
  bool ollama_fetch_enabled =
      prefs_->GetBoolean(prefs::kBraveAIChatOllamaFetchEnabled);

  if (ollama_fetch_enabled) {
    FetchModels();
  } else {
    RemoveModels();
  }
}

void OllamaModelFetcher::FetchModels() {
  ollama_client_->FetchModels(base::BindOnce(
      &OllamaModelFetcher::OnModelsResponse, weak_ptr_factory_.GetWeakPtr()));
}

void OllamaModelFetcher::OnModelsResponse(
    std::optional<std::string> response_body) {
  if (!response_body || response_body->empty()) {
    return;
  }

  std::optional<base::Value::Dict> json_dict =
      base::JSONReader::ReadDict(*response_body);
  if (!json_dict) {
    return;
  }

  const base::Value::List* models_list = json_dict->FindList("models");
  if (!models_list) {
    return;
  }

  const auto& existing_models = model_service_->GetModels();
  std::set<std::string> existing_ollama_models;

  // Find existing Ollama models to avoid duplicates
  for (const auto& existing_model : existing_models) {
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
      existing_ollama_models.insert(model_name);
    }
  }

  // Build set of current Ollama models from the response
  std::set<std::string> current_ollama_models;
  for (const auto& model : *models_list) {
    const base::Value::Dict* model_dict = model.GetIfDict();
    if (!model_dict) {
      continue;
    }

    const std::string* model_name = model_dict->FindString("name");
    if (model_name) {
      current_ollama_models.insert(*model_name);
    }
  }

  // Remove Ollama models that are no longer available
  for (const std::string& existing_model_name : existing_ollama_models) {
    if (!current_ollama_models.contains(existing_model_name)) {
      // Find and remove the model by finding its index
      for (size_t i = 0; i < existing_models.size(); ++i) {
        const auto& model = existing_models[i];
        if (model->options && model->options->is_custom_model_options() &&
            model->options->get_custom_model_options() &&
            model->options->get_custom_model_options()->endpoint.spec() ==
                mojom::kOllamaEndpoint &&
            model->options->get_custom_model_options()->model_request_name ==
                existing_model_name) {
          // Convert full model list index to custom model list index
          size_t custom_model_index = 0;
          for (size_t j = 0; j < i; ++j) {
            if (existing_models[j]->options &&
                existing_models[j]->options->is_custom_model_options()) {
              custom_model_index++;
            }
          }

          model_service_->DeleteCustomModel(custom_model_index);
          break;
        }
      }
    }
  }

  // Clear pending models map before processing new models
  pending_models_.clear();

  // Fetch detailed information for each new model
  for (const auto& model : *models_list) {
    const base::Value::Dict* model_dict = model.GetIfDict();
    if (!model_dict) {
      continue;
    }

    const std::string* model_name = model_dict->FindString("name");
    if (!model_name || existing_ollama_models.contains(*model_name)) {
      continue;
    }

    // Store basic info and fetch detailed information
    PendingModelInfo pending_info;
    pending_info.model_name = *model_name;
    pending_info.display_name = FormatOllamaModelName(*model_name);
    pending_models_[*model_name] = std::move(pending_info);

    FetchModelDetails(*model_name);
  }
}

void OllamaModelFetcher::FetchModelDetails(const std::string& model_name) {
  ollama_client_->ShowModel(
      model_name, base::BindOnce(&OllamaModelFetcher::OnModelDetailsResponse,
                                 weak_ptr_factory_.GetWeakPtr(), model_name));
}

void OllamaModelFetcher::OnModelDetailsResponse(
    const std::string& model_name,
    std::optional<std::string> response_body) {
  auto it = pending_models_.find(model_name);
  if (it == pending_models_.end()) {
    return;
  }

  uint32_t context_size = kDefaultContextSize;
  bool vision_support = false;

  // Parse the model details response
  if (response_body) {
    std::optional<base::Value::Dict> json_dict =
        base::JSONReader::ReadDict(*response_body);
    if (json_dict) {
      // Extract context_length from model_info
      const base::Value::Dict* model_info = json_dict->FindDict("model_info");
      if (model_info) {
        for (const auto [key, value] : *model_info) {
          if (base::EndsWith(key, ".context_length") && value.is_int()) {
            context_size = static_cast<uint32_t>(value.GetInt());
            break;
          }
        }
      }

      // Check capabilities for vision support
      const base::Value::List* capabilities =
          json_dict->FindList("capabilities");
      if (capabilities) {
        for (const auto& capability : *capabilities) {
          if (capability.is_string() && capability.GetString() == "vision") {
            vision_support = true;
            break;
          }
        }
      }
    }
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

void OllamaModelFetcher::RemoveModels() {
  const auto& all_models = model_service_->GetModels();
  std::vector<uint32_t> ollama_custom_indices;

  // Find custom models and identify which ones are Ollama models
  uint32_t custom_model_index = 0;
  for (size_t i = 0; i < all_models.size(); ++i) {
    const auto& model = all_models[i];

    if (model->options && model->options->is_custom_model_options()) {
      if (model->options->get_custom_model_options() &&
          model->options->get_custom_model_options()->endpoint.is_valid() &&
          model->options->get_custom_model_options()->endpoint.spec() ==
              mojom::kOllamaEndpoint) {
        ollama_custom_indices.push_back(custom_model_index);
      }
      custom_model_index++;
    }
  }

  // Remove Ollama models (in reverse order to maintain indices)
  for (auto it = ollama_custom_indices.rbegin();
       it != ollama_custom_indices.rend(); ++it) {
    model_service_->DeleteCustomModel(*it);
  }
}

// static
std::string OllamaModelFetcher::FormatOllamaModelName(
    const std::string& raw_name) {
  // Trim whitespace
  std::string_view trimmed_view =
      base::TrimWhitespaceASCII(raw_name, base::TRIM_ALL);
  if (trimmed_view.empty()) {
    return raw_name;
  }

  std::string trimmed(trimmed_view);

  // Remove :latest suffix
  if (base::EndsWith(trimmed, ":latest")) {
    trimmed = trimmed.substr(0, trimmed.length() - 7);
  }

  // Replace colons and hyphens with spaces
  base::ReplaceChars(trimmed, ":-", " ", &trimmed);

  // Split into tokens to process each part
  std::vector<std::string> parts = base::SplitString(
      trimmed, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  if (parts.empty()) {
    return raw_name;
  }

  // Process each part: capitalize first letter, handle single-letter suffixes
  std::vector<std::string> processed_parts;
  for (const std::string& part : parts) {
    std::string processed;
    bool at_start = true;

    for (size_t i = 0; i < part.length(); ++i) {
      char c = part[i];
      char prev = (i > 0) ? part[i - 1] : '\0';

      // Add space between letter and digit or digit and letter
      // but keep single letters attached (e.g., v1, 7b)
      if (i > 0 && !processed.empty()) {
        bool prev_digit = absl::ascii_isdigit(prev);
        bool curr_digit = absl::ascii_isdigit(c);
        bool prev_alpha = absl::ascii_isalpha(prev);
        bool curr_alpha = absl::ascii_isalpha(c);

        if ((prev_digit && curr_alpha) || (prev_alpha && curr_digit)) {
          // Check if it's a multi-letter word
          bool is_multi_letter = false;
          if (curr_alpha) {
            // Look ahead for more letters
            is_multi_letter =
                (i + 1 < part.length() && absl::ascii_isalpha(part[i + 1]));
          } else if (prev_alpha) {
            // Look back for more letters
            is_multi_letter = (i >= 2 && absl::ascii_isalpha(part[i - 2]));
          }

          if (is_multi_letter) {
            processed += ' ';
            at_start = true;
          }
        }
      }

      // Capitalize appropriately
      if (absl::ascii_isalpha(c)) {
        if (at_start) {
          processed += absl::ascii_toupper(c);
          at_start = false;
        } else if (i > 0 && absl::ascii_isdigit(prev)) {
          // Single letter after digit - uppercase (e.g., 7B)
          bool is_single =
              (i + 1 >= part.length() || !absl::ascii_isalpha(part[i + 1]));
          processed +=
              is_single ? absl::ascii_toupper(c) : absl::ascii_tolower(c);
        } else {
          processed += absl::ascii_tolower(c);
        }
      } else {
        processed += c;
      }
    }

    processed_parts.push_back(processed);
  }

  return base::JoinString(processed_parts, " ");
}

}  // namespace ai_chat
