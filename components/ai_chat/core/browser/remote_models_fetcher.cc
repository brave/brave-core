// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/remote_models_fetcher.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/browser/remote_models_serialization.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

constexpr size_t kMaxResponseSize = 5 * 1024 * 1024;  // 5MB

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
          "This feature can be disabled via the AIChatRemoteModelsConfig "
          "feature flag."
      })");

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
  const GURL endpoint_url(url);

  if (!endpoint_url.is_valid() || !endpoint_url.SchemeIs(url::kHttpsScheme)) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), std::vector<mojom::ModelPtr>{}));
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

void RemoteModelsFetcher::OnFetchComplete(
    FetchModelsCallback callback,
    api_request_helper::APIRequestResult result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!result.Is2XXResponseCode()) {
    std::move(callback).Run({});
    return;
  }

  std::move(callback).Run(ParseModelsFromJSON(result.value_body()));
}

}  // namespace ai_chat
