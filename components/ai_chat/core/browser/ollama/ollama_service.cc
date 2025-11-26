/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ollama/ollama_service.h"

#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/strings/string_util.h"
#include "brave/components/ai_chat/core/browser/ollama/ollama_model_fetcher.h"
#include "net/http/http_request_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace ai_chat {

namespace {

constexpr net::NetworkTrafficAnnotationTag kOllamaConnectionAnnotation =
    net::DefineNetworkTrafficAnnotation("brave_leo_assistant_ollama_connection",
                                        R"(
        semantics {
          sender: "Brave Leo Assistant"
          description:
            "Check if Ollama is running on localhost to enable fetching."
          trigger:
            "User accesses Leo Assistant settings with Ollama fetching."
          data:
            "HTTP request to localhost:11434 to check Ollama availability."
          destination: LOCAL
        }
        policy {
          cookies_allowed: NO
          setting: "This feature can be controlled in Leo Assistant settings."
        })");

constexpr net::NetworkTrafficAnnotationTag kOllamaModelsAnnotation =
    net::DefineNetworkTrafficAnnotation("brave_leo_assistant_ollama_models", R"(
        semantics {
          sender: "Brave Leo Assistant"
          description:
            "Fetch available models from local Ollama instance for chat."
          trigger:
            "User enables Ollama fetching in Leo Assistant settings."
          data:
            "HTTP request to localhost:11434/api/tags for models."
          destination: LOCAL
        }
        policy {
          cookies_allowed: NO
          setting: "This feature can be disabled in Leo Assistant settings."
        })");

constexpr net::NetworkTrafficAnnotationTag kOllamaModelDetailsAnnotation =
    net::DefineNetworkTrafficAnnotation(
        "brave_leo_assistant_ollama_model_details",
        R"(
        semantics {
          sender: "Brave Leo Assistant"
          description:
            "Fetch detailed information for a specific Ollama model."
          trigger:
            "User enables Ollama fetching in Leo Assistant settings."
          data:
            "HTTP POST request to localhost:11434/api/show with model name."
          destination: LOCAL
        }
        policy {
          cookies_allowed: NO
          setting: "This feature can be disabled in Leo Assistant settings."
        })");

// Max download sizes for Ollama API responses
constexpr size_t kConnectionCheckMaxSize = 1024;   // 1KB for connection check
constexpr size_t kModelListMaxSize = 1024 * 1024;  // 1MB for model list
constexpr size_t kModelDetailsMaxSize = 1024 * 1024;  // 1MB for model details

}  // namespace

OllamaService::OllamaService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    std::unique_ptr<OllamaModelFetcher> model_fetcher)
    : url_loader_factory_(std::move(url_loader_factory)),
      model_fetcher_(std::move(model_fetcher)) {
  if (model_fetcher_) {
    model_fetcher_->SetDelegate(this);
  }
}

OllamaService::~OllamaService() = default;

void OllamaService::Shutdown() {
  model_fetcher_.reset();
}

void OllamaService::BindReceiver(
    mojo::PendingReceiver<mojom::OllamaService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void OllamaService::IsConnected(IsConnectedCallback callback) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(mojom::kOllamaBaseUrl);
  request->method = net::HttpRequestHeaders::kGetMethod;

  auto loader = network::SimpleURLLoader::Create(std::move(request),
                                                 kOllamaConnectionAnnotation);

  auto* loader_ptr = loader.get();
  loader_ptr->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&OllamaService::OnConnectionCheckComplete,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     std::move(loader)),
      kConnectionCheckMaxSize);
}

void OllamaService::OnConnectionCheckComplete(
    IsConnectedCallback callback,
    std::unique_ptr<network::SimpleURLLoader> loader,
    std::optional<std::string> response) {
  bool connected = loader->ResponseInfo() && loader->ResponseInfo()->headers &&
                   loader->ResponseInfo()->headers->response_code() == 200;

  std::move(callback).Run(connected);
}

void OllamaService::FetchModels(ModelsCallback callback) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(mojom::kOllamaListModelsAPIEndpoint);
  request->method = net::HttpRequestHeaders::kGetMethod;

  auto loader = network::SimpleURLLoader::Create(std::move(request),
                                                 kOllamaModelsAnnotation);

  auto* loader_ptr = loader.get();
  loader_ptr->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&OllamaService::OnModelsListComplete,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     std::move(loader)),
      kModelListMaxSize);
}

void OllamaService::OnModelsListComplete(
    ModelsCallback callback,
    std::unique_ptr<network::SimpleURLLoader> loader,
    std::optional<std::string> response) {
  if (!response) {
    std::move(callback).Run(std::nullopt);
    return;
  }
  std::move(callback).Run(ParseModelsResponse(*response));
}

void OllamaService::ShowModel(const std::string& model_name,
                              ModelDetailsCallback callback) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(mojom::kOllamaShowModelInfoAPIEndpoint);
  request->method = "POST";

  auto loader = network::SimpleURLLoader::Create(std::move(request),
                                                 kOllamaModelDetailsAnnotation);

  std::string body = absl::StrFormat("{\"model\":\"%s\"}", model_name);
  loader->AttachStringForUpload(body, "application/json");

  auto* loader_ptr = loader.get();
  loader_ptr->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&OllamaService::OnModelDetailsComplete,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     std::move(loader)),
      kModelDetailsMaxSize);
}

void OllamaService::OnModelDetailsComplete(
    ModelDetailsCallback callback,
    std::unique_ptr<network::SimpleURLLoader> loader,
    std::optional<std::string> response) {
  if (!response) {
    std::move(callback).Run(std::nullopt);
    return;
  }
  std::move(callback).Run(ParseModelDetailsResponse(*response));
}

std::optional<std::vector<std::string>> OllamaService::ParseModelsResponse(
    const std::string& response_body) {
  std::optional<base::Value::Dict> json_dict = base::JSONReader::ReadDict(
      response_body, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!json_dict) {
    return std::nullopt;
  }

  const base::Value::List* models_list = json_dict->FindList("models");
  if (!models_list) {
    return std::nullopt;
  }

  std::vector<std::string> models;
  for (const auto& model : *models_list) {
    const base::Value::Dict* model_dict = model.GetIfDict();
    if (!model_dict) {
      continue;
    }

    const std::string* model_name = model_dict->FindString("name");
    if (!model_name) {
      continue;
    }

    models.push_back(*model_name);
  }

  return models;
}

std::optional<OllamaService::ModelDetails>
OllamaService::ParseModelDetailsResponse(const std::string& response_body) {
  std::optional<base::Value::Dict> json_dict = base::JSONReader::ReadDict(
      response_body, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!json_dict) {
    return std::nullopt;
  }

  ModelDetails details;

  // Extract context_length from model_info
  const base::Value::Dict* model_info = json_dict->FindDict("model_info");
  if (model_info) {
    for (const auto [key, value] : *model_info) {
      if (base::EndsWith(key, ".context_length") && value.is_int()) {
        details.context_length = static_cast<uint32_t>(value.GetInt());
        break;
      }
    }
  }

  // Check capabilities for vision support
  // See:
  // https://github.com/ollama/ollama/blob/main/docs/api.md#show-model-information
  const base::Value::List* capabilities = json_dict->FindList("capabilities");
  if (capabilities) {
    for (const auto& capability : *capabilities) {
      if (capability.is_string() && capability.GetString() == "vision") {
        details.has_vision = true;
        break;
      }
    }
  }

  return details;
}

}  // namespace ai_chat
