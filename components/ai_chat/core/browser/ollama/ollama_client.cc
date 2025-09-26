/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ollama/ollama_client.h"

#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

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

}  // namespace

OllamaClient::OllamaClient(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(std::move(url_loader_factory)) {}

OllamaClient::~OllamaClient() = default;

void OllamaClient::BindReceiver(
    mojo::PendingReceiver<mojom::OllamaService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void OllamaClient::CheckConnection(CheckConnectionCallback callback) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(mojom::kOllamaBaseUrl);
  request->method = "GET";

  auto loader = network::SimpleURLLoader::Create(std::move(request),
                                                 kOllamaConnectionAnnotation);

  auto* loader_ptr = loader.get();
  loader_ptr->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&OllamaClient::OnConnectionCheckComplete,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     std::move(loader)),
      1024);  // Small response expected
}

void OllamaClient::OnConnectionCheckComplete(
    CheckConnectionCallback callback,
    std::unique_ptr<network::SimpleURLLoader> loader,
    std::unique_ptr<std::string> response) {
  auto result = mojom::OllamaConnectionResult::New();

  if (response && response->find("Ollama is running") != std::string::npos &&
      loader->ResponseInfo() && loader->ResponseInfo()->headers &&
      loader->ResponseInfo()->headers->response_code() == 200) {
    result->connected = true;
  } else {
    result->connected = false;
    result->error = "Ollama is not running at localhost:11434";
  }

  std::move(callback).Run(std::move(result));
}

void OllamaClient::FetchModels(ModelsCallback callback) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(mojom::kOllamaApiTagsEndpoint);
  request->method = "GET";

  auto loader = network::SimpleURLLoader::Create(std::move(request),
                                                 kOllamaModelsAnnotation);

  auto* loader_ptr = loader.get();
  loader_ptr->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&OllamaClient::OnModelsListComplete,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     std::move(loader)),
      1024 * 1024);  // 1MB should be enough for model list
}

void OllamaClient::OnModelsListComplete(
    ModelsCallback callback,
    std::unique_ptr<network::SimpleURLLoader> loader,
    std::unique_ptr<std::string> response) {
  if (!response) {
    std::move(callback).Run("");
    return;
  }

  std::move(callback).Run(*response);
}

}  // namespace ai_chat
