/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ollama/ollama_service.h"

#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "net/http/http_request_headers.h"
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

// Max download size for connection check
constexpr size_t kConnectionCheckMaxSize = 1024;  // 1KB for connection check

}  // namespace

OllamaService::OllamaService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(std::move(url_loader_factory)) {}

OllamaService::~OllamaService() = default;

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

}  // namespace ai_chat
