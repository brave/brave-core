/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_settings_leo_assistant_handler.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <vector>

#include "base/containers/contains.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/model_validator.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/prefs.h"
#include "brave/browser/ai_chat/ai_chat_settings_helper.h"
#include "brave/components/ai_chat/content/browser/model_service_factory.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace {

constexpr net::NetworkTrafficAnnotationTag kOllamaConnectionAnnotation =
    net::DefineNetworkTrafficAnnotation("ollama_connection", R"(
        semantics {
          sender: "Brave Leo Assistant Settings"
          description:
            "Check if Ollama is running on localhost to enable model sync."
          trigger:
            "User clicks 'Sync with Ollama' button in Leo Assistant settings."
          data: "HTTP request to localhost:11434 to check Ollama availability."
          destination: LOCAL
        }
        policy {
          cookies_allowed: NO
          setting: "This feature can be disabled in Leo Assistant settings."
        })");

const std::vector<sidebar::SidebarItem>::const_iterator FindAiChatSidebarItem(
    const std::vector<sidebar::SidebarItem>& items) {
  return std::ranges::find_if(items, [](const auto& item) {
    return item.built_in_item_type ==
           sidebar::SidebarItem::BuiltInItemType::kChatUI;
  });
}

bool ShowLeoAssistantIconVisibleIfNot(
    sidebar::SidebarService* sidebar_service) {
  const auto hidden_items = sidebar_service->GetHiddenDefaultSidebarItems();
  const auto item_hidden_iter = FindAiChatSidebarItem(hidden_items);

  if (item_hidden_iter != hidden_items.end()) {
    sidebar_service->AddItem(*item_hidden_iter);
    return true;
  }

  return false;
}

bool HideLeoAssistantIconIfNot(sidebar::SidebarService* sidebar_service) {
  const auto visible_items = sidebar_service->items();
  const auto item_visible_iter = FindAiChatSidebarItem(visible_items);

  if (item_visible_iter != visible_items.end()) {
    sidebar_service->RemoveItemAt(item_visible_iter - visible_items.begin());
    return true;
  }

  return false;
}

}  // namespace

namespace settings {

BraveLeoAssistantHandler::BraveLeoAssistantHandler() = default;

BraveLeoAssistantHandler::~BraveLeoAssistantHandler() = default;

void BraveLeoAssistantHandler::RegisterMessages() {
  profile_ = Profile::FromWebUI(web_ui());

  web_ui()->RegisterMessageCallback(
      "toggleLeoIcon",
      base::BindRepeating(&BraveLeoAssistantHandler::HandleToggleLeoIcon,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getLeoIconVisibility",
      base::BindRepeating(&BraveLeoAssistantHandler::HandleGetLeoIconVisibility,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "resetLeoData",
      base::BindRepeating(&BraveLeoAssistantHandler::HandleResetLeoData,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "validateModelEndpoint",
      base::BindRepeating(
          &BraveLeoAssistantHandler::HandleValidateModelEndpoint,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "checkOllamaConnection",
      base::BindRepeating(
          &BraveLeoAssistantHandler::HandleCheckOllamaConnection,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "syncOllamaModels",
      base::BindRepeating(
          &BraveLeoAssistantHandler::HandleSyncOllamaModels,
          base::Unretained(this)));
}

void BraveLeoAssistantHandler::OnJavascriptAllowed() {
  sidebar_service_observer_.Reset();
  sidebar_service_observer_.Observe(
      sidebar::SidebarServiceFactory::GetForProfile(profile_));
}

void BraveLeoAssistantHandler::OnJavascriptDisallowed() {
  sidebar_service_observer_.Reset();
}

void BraveLeoAssistantHandler::OnItemAdded(const sidebar::SidebarItem& item,
                                           size_t index) {
  if (item.built_in_item_type ==
      sidebar::SidebarItem::BuiltInItemType::kChatUI) {
    NotifyChatUiChanged(true);
  }
}

void BraveLeoAssistantHandler::OnItemRemoved(const sidebar::SidebarItem& item,
                                             size_t index) {
  if (item.built_in_item_type ==
      sidebar::SidebarItem::BuiltInItemType::kChatUI) {
    NotifyChatUiChanged(false);
  }
}

void BraveLeoAssistantHandler::NotifyChatUiChanged(const bool& is_leo_visible) {
  if (!IsJavascriptAllowed()) {
    return;
  }
  FireWebUIListener("settings-brave-leo-assistant-changed", is_leo_visible);
}

void BraveLeoAssistantHandler::HandleToggleLeoIcon(
    const base::Value::List& args) {
  auto* service = sidebar::SidebarServiceFactory::GetForProfile(profile_);

  AllowJavascript();
  if (!ShowLeoAssistantIconVisibleIfNot(service)) {
    HideLeoAssistantIconIfNot(service);
  }
}

void BraveLeoAssistantHandler::HandleValidateModelEndpoint(
    const base::Value::List& args) {
  AllowJavascript();

  if (args.size() < 2 || !args[1].is_dict()) {
    // Expect the appropriate number and type of arguments, or reject
    RejectJavascriptCallback(args[0], base::Value("Invalid arguments"));
    return;
  }

  const base::Value::Dict& dict = args[1].GetDict();
  GURL endpoint(*dict.FindString("url"));

  base::Value::Dict response;

  const bool is_valid = ai_chat::ModelValidator::IsValidEndpoint(endpoint);

  response.Set("isValid", is_valid);
  response.Set("isValidAsPrivateEndpoint",
               ai_chat::ModelValidator::IsValidEndpoint(
                   endpoint, std::optional<bool>(true)));
  response.Set("isValidDueToPrivateIPsFeature",
               is_valid && ai_chat::features::IsAllowPrivateIPsEnabled() &&
                   !ai_chat::ModelValidator::IsValidEndpoint(
                       endpoint, std::optional<bool>(false)));

  ResolveJavascriptCallback(args[0], response);
}

void BraveLeoAssistantHandler::HandleGetLeoIconVisibility(
    const base::Value::List& args) {
  auto* service = sidebar::SidebarServiceFactory::GetForProfile(profile_);
  const auto hidden_items = service->GetHiddenDefaultSidebarItems();
  AllowJavascript();
  ResolveJavascriptCallback(
      args[0], !base::Contains(hidden_items,
                               sidebar::SidebarItem::BuiltInItemType::kChatUI,
                               &sidebar::SidebarItem::built_in_item_type));
}

void BraveLeoAssistantHandler::HandleResetLeoData(
    const base::Value::List& args) {
  auto* sidebar_service =
      sidebar::SidebarServiceFactory::GetForProfile(profile_);

  ShowLeoAssistantIconVisibleIfNot(sidebar_service);

  ai_chat::AIChatService* service =
      ai_chat::AIChatServiceFactory::GetForBrowserContext(profile_);
  if (!service) {
    return;
  }
  service->DeleteConversations();
  if (profile_) {
    ai_chat::SetUserOptedIn(profile_->GetPrefs(), false);
    ai_chat::prefs::DeleteAllMemoriesFromPrefs(*profile_->GetPrefs());
    ai_chat::prefs::ResetCustomizationsPref(*profile_->GetPrefs());
  }

  AllowJavascript();
}

void BraveLeoAssistantHandler::HandleCheckOllamaConnection(
    const base::Value::List& args) {
  AllowJavascript();

  if (args.size() < 1) {
    RejectJavascriptCallback(args[0], base::Value("Invalid arguments"));
    return;
  }

  // Check if Ollama is running on localhost:11434 using the native API
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL("http://localhost:11434/api/tags");
  request->method = "GET";
  request->headers.SetHeader("Content-Type", "application/json");

  auto loader = network::SimpleURLLoader::Create(std::move(request),
                                                 kOllamaConnectionAnnotation);

  auto* loader_ptr = loader.get();
  loader_ptr->DownloadToString(
      profile_->GetURLLoaderFactory().get(),
      base::BindOnce(
          [](base::WeakPtr<BraveLeoAssistantHandler> handler,
             std::string callback_id,
             std::unique_ptr<network::SimpleURLLoader> loader,
             std::unique_ptr<std::string> response) {
            if (!handler) return;

            base::Value::Dict result;
            if (response && !response->empty() &&
                loader->ResponseInfo() &&
                loader->ResponseInfo()->headers &&
                loader->ResponseInfo()->headers->response_code() == 200) {
              result.Set("connected", true);
              result.Set("error", "");
            } else {
              result.Set("connected", false);
              result.Set("error", "Could not connect to Ollama at localhost:11434");
            }

            handler->ResolveJavascriptCallback(base::Value(callback_id), result);
          },
          weak_ptr_factory_.GetWeakPtr(), args[0].GetString(),
          std::move(loader)),
      1024 * 1024);  // 1MB max response size
}

void BraveLeoAssistantHandler::HandleSyncOllamaModels(
    const base::Value::List& args) {
  AllowJavascript();

  if (args.size() < 1) {
    RejectJavascriptCallback(args[0], base::Value("Invalid arguments"));
    return;
  }

  // Fetch models from Ollama using the native API
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL("http://localhost:11434/api/tags");
  request->method = "GET";
  request->headers.SetHeader("Content-Type", "application/json");

  auto loader = network::SimpleURLLoader::Create(std::move(request),
                                                 kOllamaConnectionAnnotation);

  auto* loader_ptr = loader.get();
  loader_ptr->DownloadToString(
      profile_->GetURLLoaderFactory().get(),
      base::BindOnce(
          [](base::WeakPtr<BraveLeoAssistantHandler> handler,
             std::string callback_id,
             std::unique_ptr<network::SimpleURLLoader> loader,
             std::unique_ptr<std::string> response) {
            if (!handler) return;

            base::Value::Dict result;
            if (!response || response->empty() ||
                !loader->ResponseInfo() ||
                !loader->ResponseInfo()->headers ||
                loader->ResponseInfo()->headers->response_code() != 200) {
              result.Set("success", false);
              result.Set("error", "Could not fetch models from Ollama");
              handler->ResolveJavascriptCallback(base::Value(callback_id), result);
              return;
            }

            // Parse Ollama models response
            auto json_value = base::JSONReader::Read(*response);
            if (!json_value || !json_value->is_dict()) {
              result.Set("success", false);
              result.Set("error", "Invalid response from Ollama");
              handler->ResolveJavascriptCallback(base::Value(callback_id), result);
              return;
            }

            const base::Value::Dict& json_dict = json_value->GetDict();
            const base::Value::List* models_list = json_dict.FindList("models");
            if (!models_list) {
              result.Set("success", false);
              result.Set("error", "No models found in Ollama response");
              handler->ResolveJavascriptCallback(base::Value(callback_id), result);
              return;
            }

            // Create AI Chat settings helper to add models
            ai_chat::AIChatSettingsHelper settings_helper(handler->profile_);

            // Get existing custom models to avoid duplicates
            auto* model_service = ai_chat::ModelServiceFactory::GetForBrowserContext(handler->profile_);
            const std::vector<ai_chat::mojom::ModelPtr>& all_models = model_service->GetModels();

            std::set<std::string> existing_ollama_models;
            for (const auto& existing_model : all_models) {
              if (existing_model->options->is_custom_model_options()) {
                const auto& custom_options = existing_model->options->get_custom_model_options();
                if (custom_options->endpoint.spec() == "http://localhost:11434/v1/chat/completions") {
                  // This is an Ollama model, use the model request name to check for duplicates
                  existing_ollama_models.insert(custom_options->model_request_name);
                }
              }
            }

            int attempted_count = 0;
            for (const auto& model : *models_list) {
              const base::Value::Dict* model_dict = model.GetIfDict();
              if (!model_dict) continue;

              const std::string* model_name = model_dict->FindString("name");
              if (!model_name) continue;

              // Skip if model already exists
              if (existing_ollama_models.contains(*model_name)) {
                continue;
              }

              attempted_count++;

              // Get context size from details if available
              int context_size = 8192;  // Default context size
              const base::Value::Dict* details = model_dict->FindDict("details");
              if (details) {
                // Try to get context size from parameter_size for now
                // We could enhance this by making additional /api/show requests
                const std::string* param_size = details->FindString("parameter_size");
                if (param_size) {
                  // Parse parameter size to estimate context - this is a rough heuristic
                  if (param_size->find("0.5B") != std::string::npos || param_size->find("1B") != std::string::npos) {
                    context_size = 4096;
                  } else if (param_size->find("2B") != std::string::npos || param_size->find("3B") != std::string::npos) {
                    context_size = 8192;
                  } else if (param_size->find("7B") != std::string::npos || param_size->find("8B") != std::string::npos) {
                    context_size = 8192;
                  } else if (param_size->find("13B") != std::string::npos || param_size->find("14B") != std::string::npos || param_size->find("15B") != std::string::npos) {
                    context_size = 4096;
                  } else {
                    context_size = 8192;  // Default for larger models
                  }
                }
              }

              // Check if model supports vision - basic heuristic based on model name
              bool vision_support = false;
              std::string model_lower = *model_name;
              std::transform(model_lower.begin(), model_lower.end(), model_lower.begin(), ::tolower);
              if (model_lower.find("vision") != std::string::npos ||
                  model_lower.find("llava") != std::string::npos ||
                  model_lower.find("bakllava") != std::string::npos) {
                vision_support = true;
              }

              // Create custom model for Ollama
              auto custom_model = ai_chat::mojom::Model::New();
              custom_model->key = "";  // Must be empty for new models - ModelService will assign a key
              custom_model->display_name = *model_name;
              custom_model->vision_support = vision_support;
              custom_model->supports_tools = false;  // Ollama doesn't support tools yet
              custom_model->is_suggested_model = false;

              auto custom_options = ai_chat::mojom::CustomModelOptions::New();
              custom_options->model_request_name = *model_name;
              custom_options->endpoint = GURL("http://localhost:11434/v1/chat/completions");
              custom_options->api_key = "ollama";
              custom_options->context_size = context_size;
              custom_options->max_associated_content_length = 32000;
              custom_options->long_conversation_warning_character_limit = 32000;

              custom_model->options =
                  ai_chat::mojom::ModelOptions::NewCustomModelOptions(
                      std::move(custom_options));

              settings_helper.AddCustomModel(std::move(custom_model),
                  base::BindOnce([](ai_chat::mojom::OperationResult result) {
                    if (result != ai_chat::mojom::OperationResult::Success) {
                      LOG(ERROR) << "Failed to add Ollama model, result: " << static_cast<int>(result);
                    }
                  }));
            }

            result.Set("success", true);
            result.Set("addedCount", attempted_count);  // Report attempted count since async callbacks make exact success count difficult
            result.Set("error", "");
            handler->ResolveJavascriptCallback(base::Value(callback_id), result);
          },
          weak_ptr_factory_.GetWeakPtr(), args[0].GetString(),
          std::move(loader)),
      1024 * 1024);  // 1MB max response size
}

}  // namespace settings
